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

/**
 * \file
 * \brief Public PTP (1588) API
 * \details This header file describes the PTP (1588) control functions and types.
 */

#ifndef _VTSS_APPL_PTP_H_
#define _VTSS_APPL_PTP_H_

#include <vtss/appl/interface.h>
#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Public constants.
 */

/**
 * \brief Instance capability structure.
 *
 * This structure is used to contain the instance capability.
 *
 */
typedef struct {
    uint32_t ptp_clock_max;          /**< Max number of PTP clock instances */
} vtss_appl_ptp_capabilities_t;

/**
 * \brief Read PTP capabilities
 * \param c [out]       A pointer to a vtss_appl_ptp_capabilities_t structure in which the capabilities shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_capabilities_global_get(vtss_appl_ptp_capabilities_t *const c);

/**
 * \brief Definition of error return codes.
 * See also ptp_error_txt() in platform/ptp.c.
 */
enum {
    VTSS_APPL_PTP_RC_INV_PARAM =                            /**< Invalid parameter error returned from PTP */
            MODULE_ERROR_START(VTSS_MODULE_ID_PTP),
    VTSS_APPL_PTP_RC_INVALID_PORT_NUMBER,                   /**< Invalid port number */
    VTSS_APPL_PTP_RC_INTERNAL_PORT_NOT_ALLOWED,             /**< Enabling internal mode is only valid for a Transparent clock */
    VTSS_APPL_PTP_RC_MISSING_PHY_TIMESTAMP_RESOURCE,        /**< No timestamp engine is available in the PHY */
    VTSS_APPL_PTP_RC_MISSING_IP_ADDRESS,                    /**< cannot set up unicast ACL before my ip address is defined */
    VTSS_APPL_PTP_RC_UNSUPPORTED_ACL_FRAME_TYPE,            /**< unsupported ACL frame type */
    VTSS_APPL_PTP_RC_UNSUPPORTED_PTP_ENCAPSULATION_TYPE,    /**< unsupported PTP ancapsulation type */
    VTSS_APPL_PTP_RC_UNSUPPORTED_1PPS_OPERATION_MODE,       /**< unsupported 1PPS operation mode */
    VTSS_APPL_PTP_RC_CONFLICT_NTP_ENABLED,                  /**< cannot set system time if ntp is enabled */
    VTSS_APPL_PTP_RC_CONFLICT_PTP_ENABLED,                  /**< cannot get system time if ptp BC/Slave is enabled */
};

/**
 * \brief Definition of enum representing PPS input/output modes 
 */
typedef enum  {
    VTSS_APPL_PTP_ONE_PPS_DISABLE,      /**< 1 pps not used */
    VTSS_APPL_PTP_ONE_PPS_OUTPUT,       /**< 1 pps output */
    VTSS_APPL_PTP_ONE_PPS_INPUT,        /**< 1 pps input */
    VTSS_APPL_PTP_ONE_PPS_OUTPUT_INPUT, /**< 1 pps output and input (Jaguar1: GPIO08 is output, GPIO09 is input) */
} vtss_appl_ptp_ext_clock_1pps_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_ext_clock_1pps_txt[];  /**< enum descriptor for vtss_appl_ptp_ext_clock_1pps_t */

/**
 * \brief Definition of enum representing the frequency adjusting methods.
 */
typedef enum  {
    VTSS_APPL_PTP_PREFERRED_ADJ_LTC,         /**< Control the frequency of the internal LTC in the SWITCH/PHY's */
    VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE,      /**< Control Synce clock (DPLL) */
    VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT, /**< Control a XO independent from the Synce clock */
    VTSS_APPL_PTP_PREFERRED_ADJ_COMMON,      /**< In nodes with DUAL DPLL, Use second DPLL for PTP, Both DPLL have the same (SyncE recovered) clock */
    VTSS_APPL_PTP_PREFERRED_ADJ_AUTO,        /**< Control the select default adjustment method depending on PTP profile/Synce selector state */

} vtss_appl_ptp_preferred_adj_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_preferred_adj_txt[];   /**< enum descriptor for vtss_appl_ptp_preferred_adj_t */

/**
* \brief Definition of enum represent the unicast slave - master communication state.
*/
typedef enum  {
    VTSS_APPL_PTP_COMM_STATE_IDLE,          /**< Communication state is Idle */
    VTSS_APPL_PTP_COMM_STATE_INIT,          /**< Communication state is Initialized */
    VTSS_APPL_PTP_COMM_STATE_CONN,          /**< Communication state is Connected */
    VTSS_APPL_PTP_COMM_STATE_SELL,          /**< Communication state is Selected */
    VTSS_APPL_PTP_COMM_STATE_SYNC,          /**< Communication state is Synced */
} vtss_appl_ptp_unicast_comm_state_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_unicast_comm_state_txt[]; /**< enum descriptor for vtss_appl_ptp_unicast_comm_state_t */

/**
 * \brief Definition of enum represent the PTP Clock port state.
 */
typedef enum {
    VTSS_APPL_PTP_INITIALIZING,     /**<  PTP Clock port state is Initializing */
    VTSS_APPL_PTP_FAULTY,           /**<  PTP Clock port state is Faulty */
    VTSS_APPL_PTP_DISABLED,         /**<  PTP Clock port state is Disabled */
    VTSS_APPL_PTP_LISTENING,        /**<  PTP Clock port state is Listening */
    VTSS_APPL_PTP_PRE_MASTER,       /**<  PTP Clock port state is Pre Master */
    VTSS_APPL_PTP_MASTER,           /**<  PTP Clock port state is Master */
    VTSS_APPL_PTP_PASSIVE,          /**<  PTP Clock port state is Passive */
    VTSS_APPL_PTP_UNCALIBRATED,     /**<  PTP Clock port state is Uncalibrated */
    VTSS_APPL_PTP_SLAVE,            /**<  PTP Clock port state is Slave */
    VTSS_APPL_PTP_P2P_TRANSPARENT,  /**<  PTP Clock port state is Point to Point Transparent */
    VTSS_APPL_PTP_E2E_TRANSPARENT,  /**<  PTP Clock port state is End to End Tranparent */
    VTSS_APPL_PTP_FRONTEND          /**<  PTP Clock port state is FRONTEND */
} vtss_appl_ptp_clock_port_state_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_clock_port_state_txt[]; /**< enum descriptor for vtss_appl_ptp_clock_port_state_t */

/**
 * \brief external clock output configuration.
 *
 * This structure is used to set up the PTP external PPS and clock output.
 *
 */
typedef struct {
    vtss_appl_ptp_ext_clock_1pps_t  one_pps_mode;       /**< Select 1pps mode: */
                                                        /**< input : lock clock to 1pps input */
                                                        /**< output: enable external sync pulse output */
                                                        /**< disable: disable 1 pps */
    mesa_bool_t                            clock_out_enable;   /**< Enable programmable clock output */
                                                        /**< clock frequency = 'freq' */
    vtss_appl_ptp_preferred_adj_t   adj_method;         /**< configure the preferred adjustment method */
    uint32_t                             freq;               /**< clock output frequency (hz [1..25.000.000]). */
    uint32_t                             clk_domain;        /** clock domain number[0..2] on multi domain systems. */
} vtss_appl_ptp_ext_clock_mode_t;

/**
 * \brief Get the external clock output configuration.
 * \param mode [OUT]    A pointer to a structure containing the configuration.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_ext_clock_out_get(vtss_appl_ptp_ext_clock_mode_t *const mode);

/**
 * \brief Set the external clock output configuration.
 * \param mode [IN]     A pointer to a structure containing the configuration.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_ext_clock_out_set(const vtss_appl_ptp_ext_clock_mode_t *const mode);

/**
 * \brief define the synchconization mode for the System time <-> ptp time synchronization.
 */
typedef enum  {
    VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC,      /**< No synchronization between system clock and ptp clock */
    VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET,     /**< Update ptp clock from system clock */
    VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET,     /**< Update systerm clock from ptp clock */
} vtss_appl_ptp_system_time_sync_mode_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_system_time_sync_mode_txt[];   /**< enum descriptor for vtss_appl_ptp_system_time_sync_mode_t */

/**
 * \brief system clock to/from ptp clock synchronization mode
 */
typedef struct {
    vtss_appl_ptp_system_time_sync_mode_t     mode;   /**< Select system clock <-> ptp clock synchronization mode */
    int                                       clockinst = 0; /**<PTP clock instance <0-4>*/
} vtss_appl_ptp_system_time_sync_conf_t;

/**
 * \brief Set the synchconization mode for the System time <-> ptp time synchronization.
 * \param conf [IN]     A pointer to a structure with the configuration to set.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_system_time_sync_mode_set(const vtss_appl_ptp_system_time_sync_conf_t *const conf);

/**
 * \brief Get the synchconization mode for the System time <-> ptp time synchronization.
 * \param conf [OUT]    A pointer to a structure in which the configuration shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_system_time_sync_mode_get(vtss_appl_ptp_system_time_sync_conf_t *const conf);

/**
 * \brief main PTP Device types
 */
typedef enum {
    VTSS_APPL_PTP_DEVICE_NONE = 0,        /**< passive clock instance */
    VTSS_APPL_PTP_DEVICE_ORD_BOUND ,      /**< ordinary/bpundary clock instance*/
    VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT, /**< peer-to-peer transparent clock instance */
    VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT, /**< end-to-end transparent clock instance */
    VTSS_APPL_PTP_DEVICE_MASTER_ONLY,     /**< master only clock instance */
    VTSS_APPL_PTP_DEVICE_SLAVE_ONLY,      /**< slave only clock instance */
    VTSS_APPL_PTP_DEVICE_BC_FRONTEND,     /**< Boundary Clock frontend clock instance, only 1-step, 1-way is supported, i.e. these config parameters are ignored */
    VTSS_APPL_PTP_DEVICE_AED_GM,          /**< AED Grandmaster clock instance */
    VTSS_APPL_PTP_DEVICE_INTERNAL,        /**< Clock instance for synchronising clocks on same device. */
    VTSS_APPL_PTP_DEVICE_MAX_TYPE
} vtss_appl_ptp_device_type_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_device_type_txt[];  /**< enum descriptor for vtss_appl_ptp_device_type_t */

/**
 * \brief main PTP Protocol types
 */
typedef enum {
    VTSS_APPL_PTP_PROTOCOL_ETHERNET = 0,
    VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED = 1,
    VTSS_APPL_PTP_PROTOCOL_IP4MULTI = 2,
    VTSS_APPL_PTP_PROTOCOL_IP4MIXED = 3,
    VTSS_APPL_PTP_PROTOCOL_IP4UNI = 4,
    VTSS_APPL_PTP_PROTOCOL_OAM = 5,
    VTSS_APPL_PTP_PROTOCOL_ONE_PPS = 6,
    VTSS_APPL_PTP_PROTOCOL_IP6MIXED = 7,
    VTSS_APPL_PTP_PROTOCOL_ANY = 8,
    VTSS_APPL_PTP_PROTOCOL_MAX_TYPE
} vtss_appl_ptp_protocol_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_protocol_txt[];  /**< enum descriptor for vtss_appl_ptp_protocol_t */

/**
 * \brief Select protocol multicast destination address
 */
typedef enum {
    VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT = 0,      /**< Use default PTP multicast destination address */
    VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL = 1,   /**< Use link-local PTP multicast destination address */
    VTSS_APPL_PTP_PROTOCOL_SELECT_MAX
} vtss_appl_ptp_dest_adr_type_t;

/**
 * \brief delayMechanism values
 */
typedef enum { 
    VTSS_APPL_PTP_DELAY_MECH_E2E = 1, 
    VTSS_APPL_PTP_DELAY_MECH_P2P, 
    VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P, 
    VTSS_APPL_PTP_DELAY_MECH_DISABLED = 0xfe 
} vtss_appl_ptp_delay_mechanism_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_dest_adr_type_txt[];   /**< enum descriptor for vtss_appl_ptp_dest_adr_type_t */
extern const vtss_enum_descriptor_t vtss_appl_ptp_delay_mechanism_txt[]; /**< enum descriptor for vtss_appl_ptp_delay_mechanism_t */ 

/**
 * \brief Enumeration type listing PTP profiles
 */
typedef enum {
    VTSS_APPL_PTP_PROFILE_NO_PROFILE = 0,           /**< No profile */
    VTSS_APPL_PTP_PROFILE_1588,                     /**< IEEE 1588 PTP profile */
    VTSS_APPL_PTP_PROFILE_G_8265_1,                 /**< G8265.1 profile */
    VTSS_APPL_PTP_PROFILE_G_8275_1,                 /**< G8275.1 profile */
    VTSS_APPL_PTP_PROFILE_G_8275_2,                 /**< G8275.2 profile */
    VTSS_APPL_PTP_PROFILE_IEEE_802_1AS,             /**< IEEE 802.1AS standard profile */
    VTSS_APPL_PTP_PROFILE_AED_802_1AS               /**< Automotive Ethernet 802.1as profile */
} vtss_appl_ptp_profile_t;

/**
 * \brief AED port state
 */
typedef enum {
    VTSS_APPL_PTP_PORT_STATE_AED_MASTER = 0,
    VTSS_APPL_PTP_PORT_STATE_AED_SLAVE,
} vtss_appl_ptp_802_1as_aed_port_state_t;

/**
 * \brief Enumeration type PTP Filter types
 */
typedef enum {
    VTSS_APPL_PTP_FILTER_TYPE_ACI_DEFAULT = 0,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_XO = 1,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_XO,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_TCXO,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_TCXO,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_OCXO_S3E,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_FREQ,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_PHASE,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_FREQ,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_FDD,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_XDSL,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_FREQ,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_PHASE,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C60W,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C150,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C180,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C240,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E_R4_6_1,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE,
    VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW,
    VTSS_APPL_PTP_FILTER_TYPE_BASIC,
    VTSS_APPL_PTP_FILTER_TYPE_MAX_TYPE
} vtss_appl_ptp_filter_type_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_profile_txt[];   /**< enum descriptor for vtss_appl_ptp_profile_t */
extern const vtss_enum_descriptor_t vtss_appl_ptp_filter_type_txt[];   /**< enum descriptor for vtss_appl_ptp_filter_type_t */
extern const vtss_enum_descriptor_t vtss_appl_virtual_port_mode_txt[];
extern const vtss_enum_descriptor_t vtss_ptp_appl_rs422_protocol_txt[];

/**
 * \brief PTP clock basic configuration data (defaultDS and some vendor specific parameters.
 *
 */
typedef struct {
    vtss_appl_ptp_device_type_t  deviceType;         /**< PTP device type (see VTSS_APPL_PTP_DEVICE_xxx) */
    mesa_bool_t                         twoStepFlag;        /**< TRUE if follow up messages are used */
    uint8_t                           priority1;          /**< Priority 1 value */
    uint8_t                           priority2;          /**< Priority 2 value */
    mesa_bool_t                         oneWay;             /**< TRUE if only one way measurements are done */
    uint8_t                           domainNumber;       /**< PTP domain number */
    vtss_appl_ptp_protocol_t     protocol;           /**< Ethernet, IPv4 multicast or IPv4 unicast operation mode (see VTSS_APPL_PTP_PROTOCOL_xxx */
    mesa_vid_t                   configured_vid;     /**< vlan id used if VLAN tagging is enabled in the ethernet encapsulation mode */
                                                     /**< in IPv4 encapsulation mode, the management VLAN id is used */
    uint8_t                           configured_pcp;     /**< PCP, CFI/DEI used if VLAN tagging is enabled in the ethernet encapsulation mode */
    int32_t                          mep_instance;       /**< the mep instance number used when the protocol is OAM */
    uint32_t                          clock_domain;       /**< reference to a clock. Each clock can have it's own timing domain */
    vtss_appl_ptp_profile_t      profile;            /**< Profile to be used by this PTP clock instance */
    vtss_appl_ptp_filter_type_t  filter_type;        /**< Selects which filter/servo type is used (standard, advanced or MS-PDV) */
    uint8_t                           dscp;               /**< DSCP value used when transmitting IPv4 encapsulated packets */
    uint8_t                           localPriority;      /**< 1-255, priority used in the 8275.1 BMCA */
    mesa_bool_t                         path_trace_enable;  /**< Set to True if the Announce Path Trace is supported */
} vtss_appl_ptp_clock_config_default_ds_t;

/**
 * \brief Get a PTP clock defaultDS (configurable part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param clock_config [IN] A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_default_ds_get(uint instance, vtss_appl_ptp_clock_config_default_ds_t *const clock_config);

/**
 * \brief Set a PTP clock defaultDS (configurable part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be set.
 * \param clock_config [IN] A pointer to a structure with the configuration to set.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_default_ds_set(uint instance, const vtss_appl_ptp_clock_config_default_ds_t *clock_config);

/**
 * \brief Delete an existing PTP instance.
 * \param instance [IN]   Number of the instance to delete.
 * \return Error code.
 */
mesa_rc vtss_appl_ptp_clock_config_default_ds_del(uint instance);

/**
 * \brief Enumeration type representing types of leap seconds
 */
typedef enum {
    VTSS_APPL_PTP_LEAP_SECOND_59 = 0,           /**< Leap second event is leap59 i.e. last minute of day has only 59 seconds */
    VTSS_APPL_PTP_LEAP_SECOND_61                /**< Leap second event is leap61 i.e. last minute of day has 61 seconds */
} vtss_appl_ptp_leap_second_type_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_leap_second_type_txt[];  /**< enum descriptor for vtss_appl_ptp_leap_second_type_t */

#define CLOCK_IDENTITY_LENGTH 8         /**< Define the length in number of bytes of a clock identity */
/**
 * \brief PTP clock unique identifier
 */
typedef uint8_t vtss_appl_clock_identity [CLOCK_IDENTITY_LENGTH];

/**
 * \brief Clock Time Properties Data Set structure
 */
typedef struct {
    int16_t  currentUtcOffset;                      /**< In systems whose epoch is UTC, it is the offset between TAI and UTC */
    mesa_bool_t currentUtcOffsetValid;                 /**< When true, the value of currentUtcOffset is valid */
    mesa_bool_t leap59;                                /**< When true, this field indicates that last minute of the current UTC day has only 59 seconds */
    mesa_bool_t leap61;                                /**< When true, this field indicates that last minute of the current UTC day has 61 seconds */
    mesa_bool_t timeTraceable;                         /**< True if the timescale and the value of currentUtcOffset are traceable to a primary reference */
    mesa_bool_t frequencyTraceable;                    /**< True if the frequency determining the timescale is traceable to a primary reference */
    mesa_bool_t ptpTimescale;                          /**< True if the clock timescale of the grandmaster clock and false otherwise */
    uint8_t   timeSource;                            /**< The source of time used by the grandmaster clock */
    mesa_bool_t pendingLeap;                           /**< When true, there is a leap event pending at the date defined by leapDate */
    uint16_t  leapDate;                              /**< The date for which the leap will occur at the end of its last minute. Date is represented as the number of days after 1970-01-01 (the latter represented as 0). */
    vtss_appl_ptp_leap_second_type_t leapType;  /**< The type of leap event i.e. leap59 or leap61 */
	uint8_t stepsRemoved;
    vtss_appl_clock_identity clockIdentity[CLOCK_IDENTITY_LENGTH];    /**< Identity of the PTP clock associated with virtual port. */
} vtss_appl_ptp_clock_timeproperties_ds_t;

/**
 * \brief Get a PTP clock timepropertiesDS (configurable part).
 * \param instance [IN]             The number of the PTP clock instance for which the configuration shall be get.
 * \param timeproperties_ds [OUT]   A pointer to a structure in which the configuration shall be returned.
 * \return errorcode                An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_timeproperties_ds_get(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *const timeproperties_ds);

/**
 * \brief Set a PTP clock timepropertiesDS (configurable part).
 * \param instance [IN]            The number of the PTP clock instance for which the configuration shall be set.
 * \param timeproperties_ds [IN]   A pointer to a structure with the configuration to set.
 * \return errorcode               An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_timeproperties_ds_set(uint instance, const vtss_appl_ptp_clock_timeproperties_ds_t *const timeproperties_ds);

/**
 * \brief Clock Filter config Data Set structure
 */
typedef struct {
    uint32_t delay_filter;                   /**< Delay parameter of the delay filter */ 
    uint32_t period;                         /**< Period value of the offset filter */
    uint32_t dist;                           /**< Distance parameter of the offset filter */
} vtss_appl_ptp_clock_filter_config_t;

/**
 * \brief Get the PTP clock filter parameters.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param c [OUT]           A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_filter_parameters_get(uint instance, vtss_appl_ptp_clock_filter_config_t *const c);

/**
 * \brief Set the PTP clock filter parameters.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be set.
 * \param c [IN]            A pointer to a structure with the configuration to set.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_filter_parameters_set(uint instance, const vtss_appl_ptp_clock_filter_config_t *const c);

/**
 * \brief parameter describing PTP servo option.
 */
typedef enum {
    VTSS_APPL_PTP_CLOCK_FREE,  /**< Oscillator is freerunning */
    VTSS_APPL_PTP_CLOCK_SYNCE,  /**< Oscillator is synce locked to master */
} vtss_appl_ptp_srv_clock_option_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_srv_clock_option_txt[];   /**< enum descriptor for vtss_appl_ptp_srv_clock_option_t */

/**
 * \brief Default Clock Servo config Data Set structure
 */
typedef struct {
    mesa_bool_t                              display_stats;          /**< If true then Offset From Master, MeanPathDelay and clockAdjustment are logged on the debug terminal */
    mesa_bool_t                              p_reg;                  /**< If true the P part of the servo algorithm is included */
    mesa_bool_t                              i_reg;                  /**< If true the I part of the servo algorithm is included */
    mesa_bool_t                              d_reg;                  /**< If true the D part of the servo algorithm is included */
    uint32_t                                 ap;                     /**< The P parameter of the servo algorithm */
    uint32_t                                 ai;                     /**< The I parameter of the servo algorithm */
    uint32_t                                 ad;                     /**< The D parameter of the servo algorithm */
    uint32_t                                 gain;                   /**< The Gain parameter of the Servo algorithm. Make it possible to have gain > 1 in the servo */
    vtss_appl_ptp_srv_clock_option_t         srv_option;             /**< Determines if oscillator is free running or synhronized using SyncE */
    uint32_t                                 synce_threshold;        /**< SyncE Threshold */
    uint32_t                                 synce_ap;               /**< SyncE ap */
    int32_t                                  ho_filter;              /**< Hold off low pass filter constant for calculation of holdover frequency */
    uint64_t                                 stable_adj_threshold;   /**< The offset is assumed to be stable if the |adj_average - adj| < this value unit: ppb*10*/
} vtss_appl_ptp_clock_servo_config_t;

/**
 * \brief Get the PTP clock servo parameters.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param c [OUT]           A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_servo_parameters_get(uint instance, vtss_appl_ptp_clock_servo_config_t *const c);

/**
 * \brief Set the PTP clock servo parameters.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be set.
 * \param c [IN]            A pointer to a structure with the configuration to set.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_servo_parameters_set(uint instance, const vtss_appl_ptp_clock_servo_config_t *const c);

/**
 * \brief Clock Slave configuration Data structure
 */
typedef struct {
    uint32_t stable_offset;                  /**< Stable offset threshold in ns. */ 
    uint32_t offset_ok;                      /**< Offset OK threshold in ns. */
    uint32_t offset_fail;                    /**< Offset fail threshold in ns. */
} vtss_appl_ptp_clock_slave_config_t;

/**
 * \brief Get the PTP clock slave configuration.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param cfg [OUT]         A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_slave_config_get(uint instance, vtss_appl_ptp_clock_slave_config_t *const cfg);

/**
 * \brief Set the PTP clock slave configuration.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be set.
 * \param cfg [IN]          A pointer to a structure with the configuration to set.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_slave_config_set(uint instance, const vtss_appl_ptp_clock_slave_config_t *const cfg);

/**
 * \brief Unicast slave configuration structure
 */
typedef struct {
    uint32_t  duration;                      /**< The amount of seconds the slave requests the master to remember it */
    uint32_t  ip_addr;                       /**< The IP address of the master to which the slave shall announce itfself */
} vtss_appl_ptp_unicast_slave_config_t;

/**
 * \brief Get the PTP clock unicast slave configuration.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param idx [IN]          The index of the slave in the table
 * \param c [OUT]           A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_unicast_slave_config_get(uint instance, uint idx, vtss_appl_ptp_unicast_slave_config_t *const c);

/**
 * \brief Set the PTP clock unicast slave configuration.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be set.
 * \param idx [IN]          The index of the slave in the table
 * \param c [IN]            A pointer to a structure with the configuration to set.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_unicast_slave_config_set(uint instance, uint idx, const vtss_appl_ptp_unicast_slave_config_t *const c);

/**
 * \brief Definition of enum representing the two-step override option of the Port Data Set structure
 */
typedef enum  {
    VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE,   /**< No override. Port is using 2-step setting from clocks defaultDS structure. */
    VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE,  /**< Port is using the value false for the 2-step flag */
    VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE    /**< Port is using the value true for the 2-step flag */
} vtss_appl_ptp_twostep_override_t;

/**
 * \brief Definition of 802.1AS peer delay specific per port configuration parameters
 */
typedef struct {
    mesa_timeinterval_t meanLinkDelayThresh;                    /**< The propagation time threshold, above which a port is not considered capable of participating in the IEEE 802.1AS protocol. */
    int8_t              initialLogPdelayReqInterval;            /**< this value is the logarithm to base 2 of the Pdelay_Req message transmission interval used. */
    mesa_bool_t         useMgtSettableLogPdelayReqInterval;     /**< This value determines the source of 'currentLogPdelayReqInterval'. 'True' indicates source as 'mgtSettableLogPdelayReqInterval'. 'false' indicates source as 'LinkDelayIntervalSetting state machine'. */
    int8_t              mgtSettableLogPdelayReqInterval;        /**< This value is the logarithm to base 2 of the mean time interval between successive Pdelay_Req messages if useMgtSettableLogPdelayReqInterval is TRUE.*/
    mesa_bool_t         initialComputeNeighborRateRatio;        /**< Initial value that indicates whether neighborRateRatio is to be computed by this port. */
    mesa_bool_t         useMgtSettableComputeNeighborRateRatio; /**< This value determines the source of the value of computeNeighborRateRatio. 'True' indicates source as 'mgtSettablecomputeNeighborRateRatio'. 'false' indicates source as 'LinkDelayIntervalSetting state machine'. */
    mesa_bool_t         mgtSettableComputeNeighborRateRatio;    /**< This value indicates the input through management interface whether to compute neighbor rate ratio or not. */
    mesa_bool_t         initialComputeMeanLinkDelay;            /**< Initial value that indicates whether mean Link delay is computed by this port or not. */
    mesa_bool_t         useMgtSettableComputeMeanLinkDelay;     /**< This value determines the source of the value of computeMeanLinkDelay. 'True' indicates source as 'mgtSettableComputeMeanLinkDelay'. 'false' indicates source as 'LinkDelayIntervalSetting state machine'.*/
    mesa_bool_t         mgtSettableComputeMeanLinkDelay;        /**< This value indicates the input through management interface whether to compute Mean Link Delay or not.*/
    uint8_t             allowedLostResponses;                   /**< It is the number of Pdelay_Req messages for which a valid response is not received, above which a Link Port is considered to not be exchanging peer delay messages with its neighbor.*/
    uint8_t             allowedFaults;                          /**< It is the number of faults, above which asCapableAcrossDomains is set to FALSE.*/
    int8_t              operLogPdelayReqInterval;               /**< (AED only) This value is the logarithm to base 2 of the operational Pdelay_Req transmission interval used. */
} vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t;

/**
 * \brief Definition of 802.1AS specific pr port configuration parameters
 */
typedef struct {
    uint8_t                                     syncReceiptTimeout;         /**< Number of time-synchronization transmission intervals that a slave port waits without receiving synchronization information */
    mesa_bool_t                                 useMgtSettableLogAnnounceInterval;           /**< This value determines if managment configurable announce interval is used or not. */
    mesa_bool_t                                 useMgtSettableLogSyncInterval;               /**< This value determines if management configurable sync interval is used or not. */
    mesa_bool_t                                 useMgtSettableLogGptpCapableMessageInterval; /**< This value determinesif Management configured Gptp message interval is used or not. */
    int8_t                                      mgtSettableLogAnnounceInterval;              /**< This value indicates input through management for setting Log announce interval. */
    int8_t                                      mgtSettableLogSyncInterval;                  /**< This value indicates input through management for setting Log sync interval. */ 
    int8_t                                      mgtSettableLogGptpCapableMessageInterval;    /**< This value indicates input through management for setting GptpCapable message interval. */
    uint8_t                                     gPtpCapableReceiptTimeout;                   /**< The value of this attribute tells a port the number of gPTP capable TLV intervals to wait without receiving
from its neighbor a Signaling message containing a gPTP capable TLV, before determining that its neighbor is no longer invoking the gPTP protocol. */
    int8_t                                      initialLogGptpCapableMessageInterval;        /**< Initial value of logarithm to the base 2 of the transmission interval between successive Signaling messages that contain the gPTP capable TLV. */
    vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t peer_d;             /**< Peer delay specific configuration paramters which are part of IEEE 802.1AS protocol. */
    mesa_bool_t                                 as2020;
    // 802.1AS-AED specific port parameters
    int8_t                                      initialLogSyncInterval;                 /**< (AED only) This value is the logarithm to base 2 of the initial sync transmission interval used. */
    int8_t                                      operLogSyncInterval;                    /**< (AED only) This value is the logarithm to base 2 of the operational sync transmission interval used. */
} vtss_appl_ptp_802_1as_config_port_ds_t;

/**
 * \brief Configurable part of Port Data Set structure
 *   even though ingress- and egress-Latency and delayAsymmetry are defined pr PTP instance, the value is common for all instances.
 *   i.e. setting this value in one PTP instance will also change the value for other instances
 */
typedef struct {
    mesa_bool_t                                enabled;                    /**< disabled or enabled */
    int16_t                               logAnnounceInterval;        /**< interval between announce message transmissions */
    uint8_t                                announceReceiptTimeout;     /**< Timeout value for announce receipts */
    int16_t                               logSyncInterval;            /**< interval between sync message transmissions */
    vtss_appl_ptp_delay_mechanism_t       delayMechanism;             /**< Delay mechanism used by the port */
    int16_t                               logMinPdelayReqInterval;    /**< only for P2P deley measurements */
    mesa_timeinterval_t               delayAsymmetry;             /**< configurable delay asymmetry pr port */
    uint8_t                                portInternal;               /**< Internal(TRUE) or normal(FALSE) port */
    mesa_timeinterval_t               ingressLatency;             /**< configurable ingress delay pr port */
    mesa_timeinterval_t               egressLatency;              /**< configurable egress delay pr port */
    uint16_t                               versionNumber;              /**< PTP version used by this port */
    vtss_appl_ptp_dest_adr_type_t     dest_adr_type;              /**< Configured destinaton address for multicast packets (PTP default or LinkLocal) */
    mesa_bool_t                            masterOnly;                 /**< TRUE indicates that this interface cannot enter slave mode */
    uint8_t                                localPriority;              /**< 1-255, priority used in the 8275.1 BMCA */
    vtss_appl_ptp_twostep_override_t  twoStepOverride;            /**< Option to override the 2-step option on port level */
    mesa_bool_t                            notMaster;                   /**< TRUE indicates that this interface cannot enter master mode */
    // IEEE 802.1AS specific parameters are only available when the 802.1AS profile is selected
    vtss_appl_ptp_802_1as_config_port_ds_t c_802_1as;             /**< Defines IEEE 802.1AS specific config parameters */
    // 802.1as AED specific parameters
    vtss_appl_ptp_802_1as_aed_port_state_t aedPortState;          /**< Defines whether the port is an AED master or slave port */
} vtss_appl_ptp_config_port_ds_t;

/**
 * \brief Get a PTP clock portDS (configurable part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param portnum [IN]      The port number of the port for which the configuration shall be set.
 * \param port_ds [OUT]     A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_config_clocks_port_ds_get(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_config_port_ds_t *port_ds);

/**
 * \brief Set a PTP clock portDS (configurable part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be set.
 * \param portnum [IN]      The port number of the port for which the configuration shall be read.
 * \param port_ds [IN]      A pointer to a structure with the configuration to set.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_config_clocks_port_ds_set(uint instance, vtss_ifindex_t portnum, const vtss_appl_ptp_config_port_ds_t *port_ds);

mesa_rc vtss_appl_ptp_mgmt_config_port_ds_set(uint instance, vtss_ifindex_t portnum, const vtss_appl_ptp_config_port_ds_t *port_ds);

/**
 * \brief PTP clock Quality specification
 */
typedef struct {
    uint8_t  clockClass;                     /**< Clock class value for clock as defined in IEEE Std 1588 */
    uint8_t  clockAccuracy;                  /**< Clock accuracy value as defined in IEEE Std 1588 */
    uint16_t offsetScaledLogVariance;        /**< offsetScaledLogVariance for clock as defined in IEEE Std 1588 */
} vtss_appl_clock_quality;

typedef enum  {
    VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE,     /* virtual port not used */
    VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO,   /* virtual port main auto mode i.e 1 pps master with turn around measurement */
    VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB,         /* virtual port sub mode i.e 1 pps slave */
    VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN,    /* virtual port main man mode i.e 1 pps master */
    VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN,      /* virtual port pps-in mode i.e simple 1 pps slave */
    VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT,     /* virtual port pps-out mode i.e simple 1 pps master */
    VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT,    /* virtual port frequency-out mode i.e clock output */
} vtss_appl_virtual_port_mode_t;

typedef enum  {
    VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT, /* use serial protocol with NMEA proprietary POLYT format */
    VTSS_PTP_APPL_RS422_PROTOCOL_SER_ZDA,   /* use serial protocol with NMEA ZDA format */
    VTSS_PTP_APPL_RS422_PROTOCOL_SER_GGA,   /* use serial protocol with NMEA GGA format */
    VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC,   /* use serial protocol with NMEA RMC format */
    VTSS_PTP_APPL_RS422_PROTOCOL_PIM,       /* use PIM protocol */
    VTSS_PTP_APPL_RS422_PROTOCOL_NONE,      /* No TOD data used */
} vtss_ptp_appl_rs422_protocol_t;



/**
 * \brief Virtual port configuration structure
 */
typedef struct {
    vtss_appl_clock_quality         clockQuality;       /**< Clock quality to used when using time from virtual port (GPS) */
    uint8_t                         localPriority;      /**< 1-255, priority used in the 8275.1 BMCA */
    uint8_t                         priority1;          /**< Priority 1 value */
    uint8_t                         priority2;          /**< Priority 2 value */
    uint16_t                        input_pin;          /**< PPS input pin connected the the virtual port */
    uint16_t                        output_pin;         /**< PPS or FREQ output pin connected to the virtual port */
    uint16_t                        steps_removed;      /**< steps removed value */
    uint32_t                        freq;               /**< Frequency in Hz when FREQ output mode is selected */
    mesa_bool_t                     enable;
    vtss_appl_ptp_clock_timeproperties_ds_t timeproperties;
    uint32_t                        announce_timeout;   /**< announce timeout */
    
    vtss_appl_virtual_port_mode_t   virtual_port_mode;
    uint32_t                        delay;  			/* in MAIN mode: read only, measured turn around delay in ns.
								in SUB mode: reload value used to compensate for path delay (in ns) */
    vtss_ptp_appl_rs422_protocol_t  proto;  			/* Selected protocol */
    mesa_port_no_t                  portnum;   			/* Switch port used for the PIM protocol */
	vtss_appl_clock_identity 	    clock_identity; 		/* virtual port grand master identity */
    mesa_bool_t                     alarm;              /* Alarm generated on virtual port master device to indicate frequency state to downstream device. */
} vtss_appl_ptp_virtual_port_config_t;

/**
 * \brief Get the PTP virtual port configuration.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param c [OUT]           A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_virtual_port_config_get(uint instance, vtss_appl_ptp_virtual_port_config_t *const c);

/**
 * \brief Set the PTP virtual port configuration.
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be set.
 * \param c [IN]            A pointer to a structure with the configuration to set.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_config_virtual_port_config_set(uint instance, const vtss_appl_ptp_virtual_port_config_t *const c);

/**
 * \brief 802.1AS specific Default_DS status parameters
 */
typedef struct {
    mesa_bool_t                                gmCapable;              /**< TRUE if the time-aware system is capable of being a Grandmaster */
    uint16_t                                   sdoId;                  /**< domain identifier,12bit integer [majorSdoId,minorSdoId]*/
} vtss_appl_ptp_802_1as_status_default_ds_t;

/**
 * \brief PTP clock defaultDS status parameters.
 *
 */
typedef struct {
    uint16_t                        numberPorts;                 /**< Number of ports supported by the PTP clock */
    vtss_appl_clock_identity   clockIdentity;               /**< Identity of the PTP clock */
    vtss_appl_clock_quality    clockQuality;                /**< Quality of the PTP clock */
    vtss_appl_ptp_802_1as_status_default_ds_t s_802_1as;    /**< Defines IEEE 802.1AS specific default_DS status parameters */
} vtss_appl_ptp_clock_status_default_ds_t;

/**
 * \brief Get a PTP clock defaultDS (dynamic/status part).
 * \param instance [IN]         The number of the PTP clock instance for which the configuration shall be get.
 * \param clock_status [OUT]    A pointer to a structure in which the configuration shall be returned.
 * \return errorcode            An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_status_default_ds_get(uint instance, vtss_appl_ptp_clock_status_default_ds_t *const clock_status);

/**
 * \brief 802.1AS specific current_DS parameters
 */
typedef struct {
    mesa_scaled_ns_t        lastGMPhaseChange;              /**< The value is the phase change that occurred on the most recent change in either grandmaster or gmTimeBaseIndicator. */
    double                  lastGMFreqChange;               /**< The value is the frequency change that occurred on the most recent change in either grandmaster or gmTimeBaseIndicator. */
    uint16_t                     gmTimeBaseIndicator;            /**< timeBaseIndicator of the current grandmaster */
    uint32_t                     gmChangeCount;                  /**< The number of times the grandmaster has changed in a gPTP domain */
    uint32_t                     timeOfLastGMChangeEvent;        /**< The system time when the most recent grandmaster change occurred. */
    uint32_t                     timeOfLastGMPhaseChangeEvent;   /**< The system time when the most recent change in grandmaster phase occurred due to a change of either the grandmaster or grandmaster time base. */
    uint32_t                     timeOfLastGMFreqChangeEvent;    /**< The system time when the most recent change in grandmaster frequency occurred due to a change of either the grandmaster or grandmaster time base. */
} vtss_appl_ptp_802_1as_current_ds_t;

/**
 * \brief Clock Current Data Set structure
 */
typedef struct {
    uint16_t                 stepsRemoved;                      /**< Number of communications path between the clock and the Grandmaster clock */
    mesa_timeinterval_t offsetFromMaster;                  /**< The offset of the slave clock relative to the master as calculated by the slave */
    mesa_timeinterval_t meanPathDelay;                     /**< The mean path delay from the master to the slave as calculated by the slave */
    vtss_appl_ptp_802_1as_current_ds_t cur_802_1as;        /**< Defines IEEE 802.1AS specific current_DS parameters */
} vtss_appl_ptp_clock_current_ds_t;

/**
 * \brief Get a PTP clock currentDS (dynamic/status part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param status [OUT]      A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_status_current_ds_get(uint instance, vtss_appl_ptp_clock_current_ds_t *const status);

/**
 * \brief PTP clock port identifier
 */
typedef struct {
    vtss_appl_clock_identity    clockIdentity;              /**< Identity of the ports PTP clock */
    uint16_t                         portNumber;                 /**< Port number of the port */
} vtss_appl_ptp_port_identity;

/**
 * \brief 802.1AS specific parent_DS parameters
 */
typedef struct {
    int32_t                     cumulativeRateRatio;                        /**< The ratio of the frequency og the grandmaster to the frequencu of the Local CLock entity, expressed as fractional frequency offset * 2**41 */
} vtss_appl_ptp_802_1as_parent_ds_t;

/**
 * \brief Clock Parent Data Set structure
 */
typedef struct {
    vtss_appl_ptp_port_identity parentPortIdentity;                     /**< Identity of the parent port */
    mesa_bool_t                        parentStats;                            /**< FALSE to indicate that the next two fields are not computed. */
    uint16_t                         observedParentOffsetScaledLogVariance;  /**< optional  */
    int32_t                         observedParentClockPhaseChangeRate;     /**< optional */
    vtss_appl_clock_identity    grandmasterIdentity;                    /**< Identity of the Grandmaster */
    vtss_appl_clock_quality     grandmasterClockQuality;                /**< Clock quality of the Grandmaster */
    uint8_t                          grandmasterPriority1;                   /**< Priority 1 value of the Grandmaster */
    uint8_t                          grandmasterPriority2;                   /**< Priority 2 value of the Grandmaster */
    vtss_appl_ptp_802_1as_parent_ds_t par_802_1as;                      /**< Defines IEEE 802.1AS specific parent_DS parameters */
    int8_t                          parentLogSyncInterval;
} vtss_appl_ptp_clock_parent_ds_t;

/**
 * \brief Get a PTP clock parentDS (dynamic/status part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param status [OUT]      A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_status_parent_ds_get(uint instance, vtss_appl_ptp_clock_parent_ds_t *const status);

/**
 * \brief Get a PTP clock timepropertiesDS (dynamic/status part).
 * \param instance [IN]             The number of the PTP clock instance for which the configuration shall be get.
 * \param timeproperties_ds [OUT]   A pointer to a structure in which the configuration shall be returned.
 * \return errorcode                An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_status_timeproperties_ds_get(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds);

/**
 * \brief Slave Clock state
 */
typedef enum {
    VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREERUN,       /**< Free run state (initial) */
    VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKING,  /**< Frequency Locking */
    VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKED,   /**< Freqyency Locked */
    VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING, /**< Phase Locking */
    VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKED,  /**< Phase Locked */
    VTSS_APPL_PTP_SLAVE_CLOCK_STATE_HOLDOVER,      /**< Holdover state (if reference is lost after the stabilization period) */
    VTSS_APPL_PTP_SLAVE_CLOCK_STATE_INVALID,       /**< invalid state */
} vtss_appl_ptp_slave_clock_state_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_slave_clock_state_txt[];   /**< enum descriptor for vtss_appl_ptp_slave_clock_state_t */

/**
 * \brief PTP clock slave status structure
 */
typedef struct {
    uint16_t                                  port_number;        /**< 0 => no slave port, 1..n => selected slave port */
    vtss_appl_ptp_slave_clock_state_t    slave_state;        /**< State of the slave */
    mesa_bool_t                                 holdover_stable;    /**< true is the stabilization period has expired */
    int64_t                                  holdover_adj;       /**< the calculated holdover offset (ppb*10) */
} vtss_appl_ptp_clock_slave_ds_t;

/**
 * \brief Get a PTP clock slaveDS (dynamic/status part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param status [OUT]      A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_status_slave_ds_get(uint instance, vtss_appl_ptp_clock_slave_ds_t *const status);

/**
 * \brief PTP clock protocol identifier
 */
typedef struct {
    uint32_t ip;                             /**< IP address part of protocol address */
    mesa_mac_t mac;                     /**< MAC address part of protocol address */
} vtss_appl_ptp_protocol_adr_t;

/**
 * \brief Unicast master table structure
 */
typedef struct {
    mesa_mac_t                      mac;                /**< MAC address of the slave represented by this entry in the table */
    uint16_t                             port;               /**< Port to which the slave is connected */
    int16_t                             ann_log_msg_period; /**< the granted Announce interval */
    mesa_bool_t                            ann;                /**< true if sending announce messages */
    int16_t                             log_msg_period;     /**< the granted sync interval */
    mesa_bool_t                            sync;               /**< true if sending sync messages */
} vtss_appl_ptp_unicast_master_table_t;

/**
 * \brief Get unicast master table data.
 * \param instance [IN]           The clock instance number.
 * \param ip [IN]                 The index in the table
 * \param uni_master_table [OUT]  Pointer to a structure containing the table for the master-slave communication.
 * \return error code
 */
mesa_rc vtss_appl_ptp_clock_status_unicast_master_table_get(uint instance, uint32_t ip, vtss_appl_ptp_unicast_master_table_t *const uni_master_table);

/**
 * \brief Unicast slave table structure
 */
typedef struct {
    vtss_appl_ptp_protocol_adr_t        master;              /**< The protocol address of master to which this slave is connected */
    vtss_appl_ptp_port_identity         sourcePortIdentity;  /**< Port identity of master to which this slave is connected */
    uint16_t                                 port;                /**< Port through which the slave is connected to the master */
    int16_t                                 log_msg_period;      /**< the granted sync interval */
    vtss_appl_ptp_unicast_comm_state_t  comm_state;          /**< State of the communication with the master */
    uint32_t                                 conf_master_ip;      /**< copy of the destination ip address, used to cancel request,
                                                                  when the dest. is set to 0 */
} vtss_appl_ptp_unicast_slave_table_t;

/**
 * \brief Get unicast slave table parameters.
 * \param instance [IN]           The clock instance number.
 * \param ix [IN]                 Index in the table
 * \param uni_slave_table [OUT]   Pointer to a structure containing the table for the slave-master communication.
 * \return error code
 */
mesa_rc vtss_appl_ptp_clock_status_unicast_slave_table_get(uint instance, uint ix, vtss_appl_ptp_unicast_slave_table_t *const uni_slave_table);

/**
 * \brief define the 802.1AS port role.
 */
typedef enum  {
    VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT   = 3,      /**< Port is disabled */
    VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT     = 6,      /**< Port is in master state */
    VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT    = 7,      /**< Port is in passive state */
    VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT      = 9,      /**< Port is in slave state */
} vtss_appl_ptp_802_1as_port_role_t;

extern const vtss_enum_descriptor_t vtss_appl_ptp_802_1as_port_role_txt[];   /**< enum descriptor for vtss_appl_ptp_802_1as_port_role_t */

/**
 * \brief 802.1AS Peer Delay Service specific status parameters
 */
typedef struct {
    mesa_bool_t                 isMeasuringDelay;                /**< true if port is measuring porpagation delay. */
    int32_t                     neighborRateRatio;               /**< Estimate of the ratio of the frequency of the LocalClock entity of the time-aware system at the other end of the link attached to this Link Port, to the frequency of the LocalClock entity of this time-aware system. */
    int8_t                      currentLogPDelayReqInterval;     /**< This value is the logarithm to the base 2 of the current Pdelay_Req message transmission interval. */
    mesa_bool_t                 currentComputeNeighborRateRatio; /**< This value is the current value of computeNeighborRateRatio. */
    mesa_bool_t                 currentComputeMeanLinkDelay;     /**< This specifies the current value of computeMeanLinkDelay. */
    uint8_t                     versionNumber;                   /**< This value is set to PTP version used for this profile(10.6.2.2.4). */
    uint8_t                     minorVersionNumber;              /**< This indicates the minor version number of IEEE 1588 PTP used in the 802.1AS profile. */
} vtss_appl_ptp_802_1as_pdelay_status_ds_t;
/**
 * \brief 802.1AS specific Port status parameters
 */
typedef struct {
    vtss_appl_ptp_802_1as_port_role_t           portRole;               /**< port role of this port */
    mesa_bool_t                                 asCapable;              /**< TRUE if the time-aware system at the other end of the link is 802.1AS capable */
    int8_t                                        currentLogAnnounceInterval; /**< log2 of the current announce interval, which is either the configured initial logAnnounceInterval or the value received in an message interval request */
    int8_t                                        currentLogSyncInterval; /**< log2 of the current sync interval, which is either the configured initial logSyncInterval or the value received in an message interval request */
    mesa_timeinterval_t                         syncReceiptTimeInterval;/**< Time interval after which sync receipt timeout occurs if time-synchronization information has not been received during the interval. */
    mesa_bool_t                                 acceptableMasterTableEnabled; /**< Always FALSE */
    //u48[4]                            pDelayTruncatedTimestampsArray /**< Not supported in this release */
    //mesa_bool_t                              asymmetryMeasurementMode    /**< Asymmetry measurement is not supported */
    mesa_bool_t                              syncLocked;
    char                                     currentLogGptpCapableMessageInterval;
    vtss_appl_ptp_802_1as_pdelay_status_ds_t    peer_d;                  /**< Peer delay specific status parameters which are part of IEEE 802.1AS protocol. */
} vtss_appl_ptp_802_1as_status_port_ds_t;

/**
 * \brief Port Data Set structure
 */
typedef struct {
    vtss_appl_ptp_port_identity         portIdentity;           /**< Identity of the port */
    vtss_appl_ptp_clock_port_state_t    portState;              /**< State of the port */
    int16_t                                 logMinDelayReqInterval; /**< master announces min time between Delay_Req's */
    mesa_timeinterval_t                 peerMeanPathDelay;      /**< P2P delay estimate. = 0 if E2E delay mechanism is used (called meanLinkDelay in 802.1AS) */
    mesa_bool_t                                peer_delay_ok;          /**< false if Portmode is P2P and peer delay has not been measured */
    mesa_bool_t                                syncIntervalError;      /**< When true it indicates a sync interval error */
    // IEEE 802.1AS specific parameters are only available when the 802.1AS profile is selected
    vtss_appl_ptp_802_1as_status_port_ds_t s_802_1as;           /**< Defines IEEE 802.1AS specific status parameters */
} vtss_appl_ptp_status_port_ds_t;

/**
 * \brief Get a PTP clock portDS (dynamic/status part).
 * \param instance [IN]     The number of the PTP clock instance for which the configuration shall be get.
 * \param portnum [IN]      The number of the port for which to get the portDS.
 * \param port_ds [OUT]     A pointer to a structure in which the configuration shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_status_clocks_port_ds_get(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_status_port_ds_t *const port_ds);

mesa_rc vtss_appl_ptp_status_clocks_virtual_port_ds_get(uint instance, uint32_t portnum, vtss_appl_ptp_status_port_ds_t *const port_ds);

/**
 * \brief 802.1AS Peer delay specific statistics structure
 */
typedef struct {
    uint32_t rxPdelayRequestCount;                       /**< increments every time a Pdelay_Req message is received */
    uint32_t rxPdelayResponseCount;                      /**< increments every time a Pdelay_Resp message is received */
    uint32_t rxPdelayResponseFollowUpCount;              /**< increments every time a Pdelay_Resp_Follow_Up message is received */
    uint32_t pdelayAllowedLostResponsesExceededCount;    /**< increments every time the value of the variable lostResponses exceeds the value of the variable allowedLostResponses */
    uint32_t txPdelayRequestCount;                       /**< increments every time a Pdelay_Req message is transmitted */
    uint32_t txPdelayResponseCount;                      /**< increments every time a Pdelay_Resp message is transmitted */
    uint32_t txPdelayResponseFollowUpCount;              /**< increments every time a Pdelay_Resp_Follow_Up message is transmitted */
} vtss_appl_ptp_802_1as_port_peer_delay_statistics_t;
/**
 * \brief Port Parameter Statistics structure
 * These parameters are defined in IEEE 802.1AS clause 14.7.
 * The counters are free running counters, that are cleared when the node powers up (i.e. no persistency)
 * The counters wraps to 0 when max is reached (the counters may count at max 128 increments pr sec, i.e. the wrap around interval is at least 388 days).
 */
typedef struct {
    uint32_t rxSyncCount;                                /**< increments every time synchronization is received */
    uint32_t rxFollowUpCount;                            /**< increments every time a Follow_Up message is received */
    uint32_t rxAnnounceCount;                            /**< increments every time an Announce message is received */
    uint32_t rxPTPPacketDiscardCount;                    /**< increments every time a PTP message is discarded due to the conditions described in IEEE 802.1AS clause 14.7.8 */
    uint32_t rxDelayRequestCount;                        /**< increments every time a delay request has been received */
    uint32_t rxDelayResponseCount;                       /**< increments every time a delay reponse has been received */
    uint32_t syncReceiptTimeoutCount;                    /**< increments every time sync receipt timeout occurs */
    uint32_t announceReceiptTimeoutCount;                /**< increments every time announce receipt timeout occurs */
    uint32_t txSyncCount;                                /**< increments every time synchronization information is transmitted */
    uint32_t txFollowUpCount;                            /**< increments every time a Follow_Up message is transmitted */
    uint32_t txAnnounceCount;                            /**< increments every time an Announce message is transmitted */
    uint32_t txDelayRequestCount;                        /**< increments every time a delay request is transmitted */
    uint32_t txDelayResponseCount;                       /**< increments every time a delay response is transmitted */
    vtss_appl_ptp_802_1as_port_peer_delay_statistics_t peer_d; /**< Peer delay specific statistics which are part of IEEE 802.1AS protocol. */
} vtss_appl_ptp_status_port_statistics_t;

/**
 * \brief Get a PTP clock port parameter statistics.
 * \param instance [IN]         The number of the PTP clock instance for which the configuration shall be get.
 * \param portnum [IN]          interface index of the port for which to get the port parameter statistics.
 * \param port_statistics [OUT] A pointer to a structure in which the port parameter statistics shall be returned.
 * \return errorcode            An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_status_clocks_port_statistics_get(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_status_port_statistics_t *const port_statistics);
/**
 * \brief Get a PTP clock port parameter statistics and clear afterwards.
 * \param instance [IN]         The number of the PTP clock instance for which the configuration shall be get.
 * \param portnum [IN]          interface index of the port for which to get the port parameter statistics.
 * \param port_statistics [OUT] A pointer to a structure in which the port parameter statistics shall be returned.
 * \return errorcode            An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_status_clocks_port_statistics_get_clear(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_status_port_statistics_t *const port_statistics);

typedef enum  {
    VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE,     /**< Disable IO pin */
    VTSS_APPL_PTP_IO_MODE_ONE_PPS_OUTPUT,      /**< enable external sync pulse output */
    VTSS_APPL_PTP_IO_MODE_WAVEFORM_OUTPUT,     /**< enable external clock output frequency */
    VTSS_APPL_PTP_IO_MODE_ONE_PPS_LOAD,        /**< enable input and load time at positive edge of input signal */
    VTSS_APPL_PTP_IO_MODE_ONE_PPS_SAVE,        /**< enable input and save time at positive edge of input signal */
    VTSS_APPL_PTP_IO_MODE_MAX
} vtss_appl_ptp_ext_io_pin_cfg_t;

/** \brief external clock io configuration. */
typedef struct {
    vtss_appl_ptp_ext_io_pin_cfg_t  pin;    /**< Defines the io operation modefor the io pin */
    uint32_t                         domain; /**< clock domain [0..2] assigned to the IO pin */
    uint32_t                         freq;   /**< clock output frequency (hz [1..25.000.000]). only relevant in WAVEFORM _OUTPUT mode */
    uint16_t                         one_pps_slave_port_number; /**< PTP port corresponding to the io-pin */
} vtss_appl_ptp_ext_io_mode_t;

/**
 * \brief external clock output configuration.
 */
/**
 * \brief Get the ptp io pin configuration.
 * \param io    [IN]    IO pin number 0..3.
 * \param mode [OUT]    A pointer to a structure containing the configuration.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_io_pin_conf_get(uint32_t io_pin, vtss_appl_ptp_ext_io_mode_t *const mode);

/**
 * \brief Set the ptp io pin configuration.
 * \param io    [IN]    IO pin number 0..3.
 * \param mode [IN]     A pointer to a structure containing the configuration.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_io_pin_conf_set(uint32_t io_pin, const vtss_appl_ptp_ext_io_mode_t *const mode);


/**
 * \brief Port Control structure
 */
typedef struct {
    mesa_bool_t                        syncToSystemClock;          /**< Write a '1' to this bit syncronizes the clock to the system clock */
} vtss_appl_ptp_clock_control_t;

/**
 * \brief Get the PTP clock control structure.
 * \param instance [IN]      The number of the PTP clock instance for which the configuration shall be get.
 * \param port_control [IN]  A pointer to a structure in which the clock control structure shall be returned.
 * \return errorcode         An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_control_get(uint instance, vtss_appl_ptp_clock_control_t *const port_control);

/**
 * \brief Set the PTP clock control structure.
 * \param instance [IN]      The number of the PTP clock instance for which the configuration shall be get.
 * \param port_control [IN]  A pointer to a structure through which the control structure is transferred.
 * \return errorcode         An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_control_set(uint instance, const vtss_appl_ptp_clock_control_t *const port_control);

/**
 * \brief iterator function used for iterating over the PTP clocks
 * \param prev [IN]   A pointer to a variable holding the number of the previous clock
 * \param next [OUT]  A pointer to a variable in which the number of the next clock shall be returned.
 * \return errorcode  An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_itr(const uint *const prev, uint *const next);

/**
 * \brief iterator function used for iterating over the clocks and masters
 * \param clock_prev [IN]     A pointer to a variable holding the number of the previous clock
 * \param clock_next [OUT]    A pointer to a variable in which the number of the next clock shall be returned.
 * \param master_prev [IN]    A pointer to a variable holding the IP address of the previous master
 * \param master_next [OUT]   A pointer to a variable in which the IP address of the next master shall be returned
 * \return errorcode          An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_master_itr(const uint *const clock_prev, uint *const clock_next, const uint *const master_prev, uint *const master_next);

/**
 * \brief iterator function used for iterating over the clocks and slaves
 * \param clock_prev [IN]     A pointer to a variable holding the number of the previous clock
 * \param clock_next [OUT]    A pointer to a variable in which the number of the next clock shall be returned
 * \param slave_prev [IN]     A pointer to a variable holding the index of the previous slave within the table
 * \param slave_next [OUT]    A pointer to a variable in which the index of the next slave within the table
 * \return errorcode          An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_slave_itr(const uint *const clock_prev, uint *const clock_next, const uint *const slave_prev, uint *const slave_next);

/**
 * \brief iterator function used for iterating over the clocks and ports
 * \param clock_prev [IN]     A pointer to a variable holding the number of the previous clock
 * \param clock_next [OUT]    A pointer to a variable in which the number of the next clock shall be returned
 * \param port_prev [IN]      A pointer to a variable holding the index of the previous port within the table
 * \param port_next [OUT]     A pointer to a variable in which the index of the next port within the table
 * \return errorcode          An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_clock_port_itr(const uint *const clock_prev, uint *const clock_next, const vtss_ifindex_t *const port_prev, vtss_ifindex_t *const port_next);

/**
 * \brief 802.1AS Common Link Delay Service specific default data parameters
 */
typedef struct {
    vtss_appl_clock_identity    clockIdentity;    /**< Identity of the PTP clock associated with common link delay service. */
    uint16_t                    numberLinkPorts;  /**< number of Link Ports of the time-aware system on which the Common Mean Link Delay Service is implemented. */
    uint16_t                    sdoId;            /**< sdoID for Common Mean Link Delay Service. */
} vtss_appl_ptp_802_1as_cmlds_default_ds_t;

/**
 * \brief 802.1AS Common Link Delay Service specific Port data Configuration parameters
 */
typedef struct {
    vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t peer_d;     /**< Peer delay specific configuration parameters. */
    mesa_timeinterval_t delayAsymmetry;                     /**< the asymmetry in the propagation delay on the link attached to this port relative to the grandmaster time base, as defined in 8.3. If propagation delay asymmetry is not modeled, then delayAsymmetry is zero. */
} vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t;

/**
 * \brief 802.1AS Common Link Delay Service specific Port data status parameters
 */
typedef struct {
    vtss_appl_ptp_port_identity portIdentity;                    /**< Identity of the port. */
    mesa_bool_t                 cmldsLinkPortEnabled;            /**< true if CMLDS is enabled on the port. */
    mesa_bool_t                 asCapableAcrossDomains;          /**< per port global variable accessible by all the domains. */
    mesa_timeinterval_t         meanLinkDelay;                   /**< It is an estimate of the current one-way propagation time on the link attached to this Link Port. */
    vtss_appl_ptp_802_1as_pdelay_status_ds_t peer_d;             /**< Peer delay specific status parameters. */
} vtss_appl_ptp_802_1as_cmlds_status_port_ds_t;

/**
 * \brief 802.1AS Common Link Delay Service specific Port Statistics data parameters defined in IEEE 802.1AS Table 14-19.
 */
typedef struct {
    uint32_t rxPTPPacketDiscardCount;                           /**< increments every time a PTP message of Common mean Link Delay Service is discarded. */
    vtss_appl_ptp_802_1as_port_peer_delay_statistics_t peer_d;  /**< Peer delay specific statistics parameters. */
} vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t;

/**
 * \brief Get PTP CMLDS port status. 
 * \param portnum [IN]      The port number of the port for which the status need to be obtained.
 * \param port_ds [OUT]     A pointer to a structure in which the CMLDS port status shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_cmlds_port_status_get(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_status_port_ds_t *const port_ds);
/**
 * \brief Get PTP CMLDS port statistics.
 * \param portnum [IN]      The port number of the port for which the statis need to be obtained.
 * \param statistics [OUT]  A pointer to a structure in which the CMLDS port statistics shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_cmlds_port_statistics_get(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t *const statistics);
/**
 * \brief Get PTP CMLDS port statistics and clear the statistics.
 * \param portnum [IN]      The port number of the port for which the statistics need to be obtained and cleared.
 * \param statistics [OUT]  A pointer to a structure in which the CMLDS port statistics shall be returned.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_cmlds_port_statistics_get_clear(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t *const statistics);
/**
 * \brief Get PTP CMLDS port Configuration.
 * \param portnum [IN]      The port number of the port for which CMLDS configuration need to be obtained.
 * \param port_ds [OUT]     A pointer to a structure in which contains CMLDS configuration.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_cmlds_port_conf_get(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *const conf);
/**
 * \brief Set PTP CMLDS port Configuration.
 * \param portnum [IN]      The port number of the port for which CMLDS configuration need to be applied.
 * \param port_ds [OUT]     A pointer to a structure in which contains CMLDS configuration.
 * \return errorcode        An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_cmlds_port_conf_set(vtss_uport_no_t portnum, const vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *const conf);

/**
 * \brief Get PTP CMLDS Default DS. 
 * \param def_ds[OUT]        A pointer to structure through default DS is obtained.
 */
mesa_rc vtss_appl_ptp_cmlds_default_ds_get(vtss_appl_ptp_802_1as_cmlds_default_ds_t *const def_ds);

/**
 * \brief iterator function used for iterating over the ports
 * \param port_prev [IN]      A pointer to a variable holding the index of the previous port within the table
 * \param port_next [OUT]     A pointer to a variable in which the index of the next port within the table
 * \return errorcode          An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_ptp_port_itr(const vtss_uport_no_t *port_prev, vtss_uport_no_t *const port_next);
#ifdef __cplusplus
}
#endif
#endif /* _VTSS_APPL_PTP_H_ */
