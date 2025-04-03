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

/**
 * \file
 * \brief Public SyncE API
 * \details This header file describes the SyncE control functions and types.
 */

#ifndef _VTSS_APPL_SYNCE_H_
#define _VTSS_APPL_SYNCE_H_

#include <vtss/appl/interface.h>
#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Instance capability structure.
 *
 * This structure contains the SyncE capabilities.
 */
typedef struct {
    uint32_t synce_source_count;          /**< Number of SyncE sources supported by the device */
    uint32_t dpll_type;                   /**< DPLL chip type (see vtss_zl_30380_dpll_type_t)  */
    uint32_t clock_type;                  /**< Clock type                                      */
    uint32_t dpll_fw_ver;                 /**< Firmware version of DPLL chip                   */
} vtss_appl_synce_capabilities_t;

/**
 * \brief Get SyncE capabilities
 * \param c             A pointer to a vtss_appl_synce_capabilities_t structure in which the capabilities shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_capabilities_global_get(vtss_appl_synce_capabilities_t *const c);

/**
 * \brief Enum defining values for the selector modes.
 */
typedef enum
{
    VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL,                   /**< Manual */
    VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED,       /**< Manual to selected */
    VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE,   /**< Automatic non-revertive */
    VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE,      /**< Automatic revertive */
    VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER,          /**< Forced holdover */
    VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN           /**< Forced free run */
} vtss_appl_synce_selection_mode_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_selection_mode_txt[];   /**< enum descriptor for vtss_appl_synce_selection_mode_t */

/**
 * \brief Enum defining values for the quality levels.
 */
typedef enum
{
    VTSS_APPL_SYNCE_QL_NONE,    /**< No overwrite */
    VTSS_APPL_SYNCE_QL_PRC,     /**< Primary Reference Clock */
    VTSS_APPL_SYNCE_QL_SSUA,    /**< Synchronization supply unit */
    VTSS_APPL_SYNCE_QL_SSUB,    /**< Synchronization supply unit */
    VTSS_APPL_SYNCE_QL_EEC1,    /**< SyncE EEC option 1 */
    VTSS_APPL_SYNCE_QL_DNU,     /**< Do Not Use */
    VTSS_APPL_SYNCE_QL_INV,     /**< receiving invalid SSM (not defined) - NOT possible to set */
    VTSS_APPL_SYNCE_QL_FAIL,    /**< NOT received SSM in five seconds - NOT possible to set */
    VTSS_APPL_SYNCE_QL_LINK,    /**< Invalid SSM due to link down - NOT possible to set */
    VTSS_APPL_SYNCE_QL_PRS,     /**< Primary Reference Source */
    VTSS_APPL_SYNCE_QL_STU,     /**< Synchronization Traceability Unknown */
    VTSS_APPL_SYNCE_QL_ST2,     /**< Stratum 2*/
    VTSS_APPL_SYNCE_QL_TNC,     /**< Transit Node Clock */
    VTSS_APPL_SYNCE_QL_ST3E,    /**< Stratum 3E */
    VTSS_APPL_SYNCE_QL_EEC2,    /**< SyncE EEC option 2 */
    VTSS_APPL_SYNCE_QL_SMC,     /**< SONET Minimum Clock */
    VTSS_APPL_SYNCE_QL_PROV,    /**< Provisionable by network operator */
    VTSS_APPL_SYNCE_QL_DUS      /**< Don't Use for Sync */
} vtss_appl_synce_quality_level_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_quality_level_txt[];   /**< enum descriptor for vtss_appl_synce_quality_level_t */

/**
 * \brief Enum defining values for the EEC options.
 *
 */
typedef enum
{
    VTSS_APPL_SYNCE_EEC_OPTION_1,  /**< EEC option 1: See ITU-T G.8262/Y.1362 */
    VTSS_APPL_SYNCE_EEC_OPTION_2   /**< EEC option 2  */
} vtss_appl_synce_eec_option_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_eec_option_txt[];   /**< enum descriptor for vtss_enum_descriptor_t */

/**
 * \brief SyncE configuration parameters related to the selection mode.
 *
 */
typedef struct
{
    vtss_appl_synce_selection_mode_t  selection_mode;                      /**< selection mode */
    uint                              source;                              /**< nominated source for manual selection mode. Range is from 1 to synce_source_count (from capabilities). */
    uint                              wtr_time;                            /**< WTR timer value in minutes. Range is from 0 (disabled) to 12 minutes (in steps of 1 minute). */
    vtss_appl_synce_quality_level_t   ssm_holdover;                        /**< tx overwrite SSM used when clock controller is hold over */
    vtss_appl_synce_quality_level_t   ssm_freerun;                         /**< tx overwrite SSM used when clock controller is free run */
    vtss_appl_synce_eec_option_t      eec_option;                          /**< Synchronous Ethernet Equipment Clock option */
} vtss_appl_synce_clock_selection_mode_config_t;

/**
 * \brief Set SyncE clock selection mode config
 * \param config [IN]   A pointer to a vtss_appl_synce_clock_selection_mode_config_t structure containing the configuration to be set.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_selection_mode_config_set(const vtss_appl_synce_clock_selection_mode_config_t *const config);

/**
 * \brief Get SyncE clock selection mode config
 * \param config [OUT]  A pointer to a vtss_appl_synce_clock_selection_mode_config_t structure in which the configuration shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_selection_mode_config_get(vtss_appl_synce_clock_selection_mode_config_t *const config);

/**
 * \brief Enum defining values for the 'aneg' setting.
 *
 */
typedef enum
{
    VTSS_APPL_SYNCE_ANEG_NONE,                    /**< No auto negotiation */
    VTSS_APPL_SYNCE_ANEG_PREFERED_SLAVE,          /**< Prefered slave */
    VTSS_APPL_SYNCE_ANEG_PREFERED_MASTER,         /**< Prefered master */
    VTSS_APPL_SYNCE_ANEG_FORCED_SLAVE,            /**< Forced slave */
} vtss_appl_synce_aneg_mode_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_aneg_mode_txt[];   /**< enum descriptor for vtss_appl_synce_aneg_mode_t */

/**
 * \brief SyncE configuration parameters related to the source nomination.
 *
 */
typedef struct
{  
    mesa_bool_t                      nominated;       /**< false = sources is not nominated */
    vtss_ifindex_t                   network_port;    /**< Network interface port used by the nominated sources */
    uint8_t                          clk_in_port;     /**< Number indicating the dedicated clk input used if network_port == 0.
                                                           At present only one station clock is supported, which is represented by the value 0.
                                                           PTP clocks are represented as the clock-id + 128 */
    uint                             priority;        /**< priority of the nominated source. Range is from 0 to the number of nominated sources minus one. */
    vtss_appl_synce_aneg_mode_t      aneg_mode;       /**< Autonegotiation mode auto-master-slave */
    vtss_appl_synce_quality_level_t  ssm_overwrite;   /**< SSM overwrite quality */
    uint                             holdoff_time;    /**< Hold Off timer value in 100ms (3 - 18). Zero means no hold off */
} vtss_appl_synce_clock_source_nomination_config_t;

/**
 * Structure holding the possible ports that can be used for nominiation.
 */
typedef struct
{
    vtss_ifindex_t                   network_port;    /**< Network interface port used by the nominated sources */
    uint8_t                          clk_in_port;     /**< Number indicating the dedicated clk input used if network_port == 0.
                                                           At present only one station clock is supported, which is represented by the value 0.
                                                           PTP clocks are represented as the clock-id + 128 */
} vtss_appl_synce_clock_possible_source_t;


/**
 * \brief Resets entire SyncE configuration to default
 */
void vtss_appl_synce_reset_configuration_to_default();

/**
 * \brief Set SyncE clock source nomination config
 * \param sourceId [IN]  The source for which the configuration shall be set.
 * \param config [IN]    A pointer to a vtss_appl_synce_clock_source_nomination_config_t structure containing the configuration to be set.
 * \return errorcode     An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_source_nomination_config_set(const uint sourceId, const vtss_appl_synce_clock_source_nomination_config_t *const config);

/**
 * \brief Get SyncE clock source nomination config
 * \param sourceId [IN]  The source for which the configuration shall be get.
 * \param config [OUT]   A pointer to a vtss_appl_synce_clock_source_nomination_config_t structure in which the configuration shall be returned.
 * \return errorcode     An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_source_nomination_config_get(const uint sourceId, vtss_appl_synce_clock_source_nomination_config_t *const config);

/**
 * \brief Get for each syncE clock source the possible ports that can be used for nomination
 * \param sourceId [IN]  The source for which the ports shall be found
 * \param index    [IN]  Index of the source port to be found
 * \param possible_source [OUT] Description of the source
 * \return errorcode     mesa_rc_ok when a there either is a network port or a clock port associated with the (sourceId, index). Otherwise mesa_rc_error.
 */
mesa_rc vtss_appl_synce_clock_source_ports_get(const uint sourceId, const uint index, vtss_appl_synce_clock_possible_source_t *possible_source);

/**
 * \brief iterator function used for iterating over the SyncE sources
 * \param prev_sourceId [IN]   A pointer to a variable holding the number of the previous source
 * \param next_sourceId [OUT]  A pointer to a variable in which the number of the next source shall be returned.
 * \param prev_index [IN] A pointer to a variable holding the number of the previous index
 * \param next_index [IN] A pointer to a variable holding the number of the next index
 * \return errorcode  An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_source_ports_itr(const uint *prev_sourceId, uint *next_sourceId, const uint *prev_index, uint *next_index);


/**
 * \brief Enum defining values for the selector state.
 */
typedef enum
{
    VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED,       /**< Clock is in the 'locked' state */
    VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER,     /**< Clock is in the 'holdover' state */
    VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN,      /**< Clock is in the 'freerunning' state */
    VTSS_APPL_SYNCE_SELECTOR_STATE_PTP,          /**< Clock is in the 'PTP' state */
    VTSS_APPL_SYNCE_SELECTOR_STATE_REF_FAILED,   /**< the selected reference failed */
    VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING     /**< Clock is acquiring lock to the selected reference */
} vtss_appl_synce_selector_state_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_selector_state_txt[];  /**< enum descriptor for vtss_appl_synce_selector_state_t */

/**
 * \brief Enum defining values for the station in and out frequency settings.
 */
typedef enum
{
    VTSS_APPL_SYNCE_STATION_CLK_DIS,            /**< Disabled */
    VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ,       /**< 1544 KHz */
    VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ,       /**< 2048 KHz */
    VTSS_APPL_SYNCE_STATION_CLK_10_MHZ,         /**< 10 MHz */
    VTSS_APPL_SYNCE_STATION_CLK_MAX,            /**< Indicates upper limit of this enum */
} vtss_appl_synce_frequency_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_frequency_txt[];  /**< enum descriptor for vtss_appl_synce_frequency_t */

/**
 * \brief SyncE configuration parameters for station clock in/out.
 */
typedef struct
{  
    vtss_appl_synce_frequency_t  station_clk_out;    /**< Station clock output frequency setting */
    vtss_appl_synce_frequency_t  station_clk_in;     /**< Station clock input frequency setting */
} vtss_appl_synce_station_clock_config_t;

/**
 * \brief Set SyncE station clock config
 * \param config [IN]   A pointer to a vtss_appl_synce_station_clock_config_t structure containing the configuration to be set.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_station_clock_config_set(const vtss_appl_synce_station_clock_config_t *const config);

/**
 * \brief Get SyncE station clock config
 * \param config [OUT]  A pointer to a vtss_appl_synce_station_clock_config_t structure containing in which the configuration shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_station_clock_config_get(vtss_appl_synce_station_clock_config_t *const config);

/**
 * \brief SyncE port configuration parameters.
 */
typedef struct
{
    mesa_bool_t                         ssm_enabled;        /**< quality level via SSM enabled */
} vtss_appl_synce_port_config_t;

/**
 * \brief Set SyncE port config
 * \param portId [IN]   The port for which the configuration shall be set.
 * \param config [IN]   A pointer to a vtss_appl_synce_port_config_t structure containing the configuration to be set.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_port_config_set(const vtss_ifindex_t portId, const vtss_appl_synce_port_config_t *const config);  

/**
 * \brief Get SyncE port config
 * \param portId [IN]   The port for which the configuration shall be get.
 * \param config [OUT]  A pointer to a vtss_appl_synce_port_config_t structure in which the configuration shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_port_config_get(const vtss_ifindex_t portId, vtss_appl_synce_port_config_t *const config);

/**
 * \brief Enum defining values for the LOL alarm state.
 */
typedef enum {
    VTSS_APPL_SYNCE_LOL_ALARM_STATE_FALSE,
    VTSS_APPL_SYNCE_LOL_ALARM_STATE_TRUE,
    VTSS_APPL_SYNCE_LOL_ALARM_STATE_NA
} vtss_appl_synce_lol_alarm_state_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_lol_alarm_state_txt[];   /**< enum descriptor for vtss_appl_synce_lol_alarm_state_t */
extern const vtss_enum_descriptor_t vtss_appl_synce_clock_hw_id_txt[];   /**< enum descriptor for meba_synce_clock_hw_id_t */


/**
 * \brief SyncE status for selection mode.
 */
typedef struct
{
    uint clock_input;                                       /**< The clock source locked to when clock selector is in locked state. */
    vtss_appl_synce_selector_state_t selector_state;        /**< This is indicating the state of the clock selector. */
    mesa_bool_t    losx;                                           /**< LOSX */
    vtss_appl_synce_lol_alarm_state_t lol;                  /**< Clock selector has raised the Los Of Lock alarm. */
    mesa_bool_t    dhold;                                          /**< Clock selector has not yet calculated the holdover frequency offset to local oscillator. */
} vtss_appl_synce_clock_selection_mode_status_t;

/**
 * \brief Get SyncE clock selection mode status
 * \param status [OUT]  A pointer to a vtss_appl_synce_clock_selection_mode_status_t structure in which the status shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_selection_mode_status_get(vtss_appl_synce_clock_selection_mode_status_t *const status);

/**
 * \brief SyncE status for source nomination.
 *
 */
typedef struct
{
    mesa_bool_t    locs;             /**< Signal is lost on this clock source. */
    mesa_bool_t    fos;              /**< FOS */
    mesa_bool_t    ssm;              /**< SSM */
    mesa_bool_t    wtr;              /**< WTR */
} vtss_appl_synce_clock_source_nomination_status_t;

/**
 * \brief Get SyncE clock source nomination status
 * \param sourceId [IN]  The source for which the status shall be get.
 * \param status [OUT]   A pointer to a vtss_appl_synce_clock_source_nomination_status_t structure in which the status shall be returned.
 * \return errorcode     An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_source_nomination_status_get(const uint sourceId, vtss_appl_synce_clock_source_nomination_status_t *const status);

/**
 * \brief SyncE port status.
 *
 */
typedef struct
{
    vtss_appl_synce_quality_level_t ssm_rx;       /**< Monitoring of the received SSM QL on this port */
    vtss_appl_synce_quality_level_t ssm_tx;       /**< Monitoring of the transmitted SSM QL on this port */
    mesa_bool_t                     master;       /**< This field is true when the port is master and false when the port is slave */
} vtss_appl_synce_port_status_t;

/**
 * \brief Get SyncE port status
 * \param portId [IN]   The port for which the status shall be get.
 * \param status [OUT]  A pointer to a vtss_appl_synce_port_status_t structure in which the configuration shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_port_status_get(const vtss_ifindex_t portId, vtss_appl_synce_port_status_t *const status);

/**
 * \brief Enum defining values for the PTSF levels.
 */
typedef enum {
    SYNCE_PTSF_NONE,                              /**< None */
    SYNCE_PTSF_UNUSABLE,                          /**< Unusable (e.g. because of excessive PDV) */
    SYNCE_PTSF_LOSS_OF_SYNC,                      /**< Loss of Synce */
    SYNCE_PTSF_LOSS_OF_ANNOUNCE                   /**< Loss of Announce */
} vtss_appl_synce_ptp_ptsf_state_t;

extern const vtss_enum_descriptor_t vtss_appl_synce_ptp_ptsf_state_txt[];  /**< enum descriptor for vtss_appl_synce_ptp_ptsf_state_t */

/**
 * \brief SyncE ptp status.
 *
 */
typedef struct
{
    vtss_appl_synce_quality_level_t ssm_rx;       /**< Monitoring of the received SSM QL on this port */
    vtss_appl_synce_ptp_ptsf_state_t ptsf;        /**< PTSF state for this port */
} vtss_appl_synce_ptp_port_status_t;

/**
 * \brief Get SyncE PTP port status
 * \param sourceId [IN] The PTP port (i.e. PTP instance) for which the status shall be get.
 * \param status [OUT]  A pointer to a vtss_appl_synce_port_status_t structure in which the configuration shall be returned.
 * \return errorcode    An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_ptp_port_status_get(const uint sourceId, vtss_appl_synce_ptp_port_status_t *const status);

/**
 * \brief Source Control structure
 */
typedef struct {
    mesa_bool_t clearWtr;                      /**< When true is written, Wtr is cleared. */
} vtss_appl_synce_clock_source_control_t;

/**
 * \brief Perform SyncE source control action
 * \param sourceId [IN]       The source for which the source control action shall be performed.
 * \param port_control [IN]   A pointer to a vtss_appl_synce_source_control_t structure in which the control operation is defined.
 * \return errorcode          An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_source_control_set(uint sourceId, const vtss_appl_synce_clock_source_control_t *const port_control);

/**
 * \brief iterator function used for iterating over the SyncE sources
 * \param prev [IN]   A pointer to a variable holding the number of the previous source
 * \param next [OUT]  A pointer to a variable in which the number of the next source shall be returned.
 * \return errorcode  An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_source_itr(const uint *const prev, uint *const next);

/**
 * \brief iterator function used for iterating over the SyncE ports
 * \param prev [IN]   A pointer to a variable holding the number of the previous port
 * \param next [OUT]  A pointer to a variable in which the number of the next port shall be returned.
 * \return errorcode  An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_port_itr(const vtss_ifindex_t *const prev, vtss_ifindex_t *const next);

/**
 * \brief iterator function used for iterating over the PTP instances when used as sources for SyncE
 * \param prev [IN]   A pointer to a variable holding the number of the previous source
 * \param next [OUT]  A pointer to a variable in which the number of the next source shall be returned.
 * \return errorcode  An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_ptp_source_itr(const uint *const prev, uint *const next);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_APPL_SYNCE_H_ */
