/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_SYNCE_H_
#define _VTSS_SYNCE_H_

#include "synce_api.h"
#include "synce_custom_clock_api.h"
#include "synce_constants.h"
#include "main.h"

#if defined(VTSS_SW_OPTION_PTP)
#include "ptp_constants.h"
#else
#define PTP_CLOCK_INSTANCES 0
#endif

#if defined(VTSS_SW_OPTION_PTP)
#include "synce_ptp_if.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SYNCE_WTR_MAX          12                           /* maximum value for wtr timer */
#define SYNCE_HOLDOFF_MIN       3                           /* minimum (except zero) value for hold off timer */
#define SYNCE_HOLDOFF_MAX      18                           /* maximum value for hold off timer */
#define SYNCE_HOLDOFF_TEST    100                           /* Test value for hold off timer */

/* ssm codes according to EEC_OPTION1 */
#define SSM_QL_PRC       0x02
#define SSM_QL_SSUB      0x08
#define SSM_QL_SEC       0x0B
    
/* ssm codes according to EEC_OPTION2 */
#define SSM_QL_PRS_INV   0x01
#define SSM_QL_STU       0x00
#define SSM_QL_ST2       0x07
#define SSM_QL_ST3E      0x0D
#define SSM_QL_ST3       0x0A
#define SSM_QL_SMC       0x0C
#define SSM_QL_PROV      0x0E

/* ssm codes common to both EEC_OPTION1 & EEC_OPTION2 */
#define SSM_QL_SSUA_TNC  0x04
#define SSM_QL_DNU_DUS   0x0F
#define SSM_QL_FAIL      0xFF

/* Link fail indication */
#define SSM_QL_LINK      0xF0

#define SSM_QL_EEC2      0x0A
#define SSM_QL_EEC1      0x0B


#define SYNCE_RC(expr) { mesa_rc my_ptp_rc = (expr); if (my_ptp_rc < VTSS_RC_OK) { \
T_W("Error code: %s", error_txt(my_ptp_rc)); }}

#define SYNCE_RETURN(expr) { mesa_rc my_ptp_rc = (expr); if (my_ptp_rc < VTSS_RC_OK) return my_ptp_rc; }


/****************************************************************************/
// API Error Return Codes (mesa_rc)
/****************************************************************************/
const char *synce_error_txt(mesa_rc rc); // Convert Error code to text
enum {
    SYNCE_ERROR_INV_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_SYNCE), // NULL parameter passed to one of the mirror_XXX functions, where a non-NULL was expected.
    SYNCE_RC_INVALID_PARAMETER,
    SYNCE_RC_NOM_PORT,
    SYNCE_RC_SELECTION,
    SYNCE_RC_INVALID_PORT,
    SYNCE_RC_NOT_SUPPORTED
};
#define SYNCE_RC_OK                 0

extern uint synce_my_nominated_max;                     /* actual max number of nominated ports */
extern uint synce_my_priority_max;                      /* actual max number of priorities */

// Enum for selecting ssm type
typedef enum {
  HOLDOVER,      // Configuration of ssm hodover
  FREERUN,       // Configuration of ssm freerun
  OVERWRITE,     // Configuration of ssm 
} synce_icli_ssm_type_t;
                                                
mesa_rc synce_mgmt_nominated_priority_set(const uint     source,     /* nominated source - range is 0 - SYNCE_NOMINATED_MAX */
                                          const uint     priority);  /* priority 0 is highest */

mesa_rc synce_mgmt_ssm_set(const uint    port_no,
                           const bool    ssm_enabled);

mesa_rc synce_mgmt_wtr_clear_set(const uint     source);     /* nominated source - range is 0 - SYNCE_NOMINATED_MAX */
#if defined(VTSS_SW_OPTION_PTP)
int synce_mgmt_get_best_master();
#endif

uint synce_mgmt_station_clock_out_set(const vtss_appl_synce_frequency_t freq); /* set the station clock output frequency */
uint synce_mgmt_station_clock_out_get(vtss_appl_synce_frequency_t *const freq);

uint synce_mgmt_station_clock_in_set(const vtss_appl_synce_frequency_t freq); /* set the station clock input frequency */
uint synce_mgmt_station_clock_in_get(vtss_appl_synce_frequency_t *const freq);
uint synce_mgmt_station_clock_type_get(uint *const clock_type);
uint synce_mgmt_eec_option_type_get(uint *const eec_type);

typedef struct
{
    bool locs[SYNCE_NOMINATED_MAX];
    bool fos[SYNCE_NOMINATED_MAX];
    bool ssm[SYNCE_NOMINATED_MAX];
    bool wtr[SYNCE_NOMINATED_MAX];
    bool losx;
    vtss_appl_synce_lol_alarm_state_t lol;
    bool dhold;
    bool trans;  /* true if a transient is active */
} synce_mgmt_alarm_state_t;

mesa_rc synce_mgmt_port_state_get(const uint                      port_no,
                                  vtss_appl_synce_quality_level_t *const ssm_rx,
                                  vtss_appl_synce_quality_level_t *const ssm_tx,
                                  bool                            *const master);

const char* ssm_string(vtss_appl_synce_quality_level_t ssm);
/* helper function to convert raw ssm_code to quality level */
vtss_appl_synce_quality_level_t ssm_to_ql(uint ssm ,vtss_appl_synce_eec_option_t eec_option);

uint synce_mgmt_register_get(const uint reg, uint *const value);
uint synce_mgmt_register_set(const uint reg, const uint value);
mesa_rc synce_subject_debug_write(char *subject_name, u32 index, char *value);

void synce_mgmt_set_clock_source_nomination_config_to_default(vtss_appl_synce_clock_source_nomination_config_t *clock_source_nomination_config);
void synce_get_clock_selection_mode_config_default(vtss_appl_synce_clock_selection_mode_config_t *config);
void synce_set_clock_selection_mode_config_to_default(vtss_appl_synce_clock_selection_mode_config_t *clock_selection_mode_config);
void synce_mgmt_set_station_clock_config_to_default(vtss_appl_synce_station_clock_config_t *station_clock_config);
void synce_mgmt_set_port_config_to_default(vtss_appl_synce_port_config_t *port_config);

mesa_rc vtss_synce_clock_source_nomination_config_all_get(vtss_appl_synce_clock_source_nomination_config_t *const config);

bool clock_out_range_check(const vtss_appl_synce_frequency_t freq);
bool clock_in_range_check(const vtss_appl_synce_frequency_t freq);

/**
 * \brief Dummy function needed by SNMP as it does not support write-only objects (always returns NULL data).
 * \param sourceId [IN]       The source for which the status shall be get.
 * \param port_control [OUT]  A pointer to a vtss_appl_synce_source_control_t structure in which the dummy data shall be returned.
 * \return errorcode          An errorcode of type mesa_rc
 */
mesa_rc vtss_appl_synce_clock_source_control_get(uint sourceId, vtss_appl_synce_clock_source_control_t *const port_control);

// See synce.c
u8 synce_iclk2uclk(u8 iclk);

// See synce.c
u8 synce_uclk2iclk(u8 uclk);

// See synce.c
mesa_rc synce_network_port_clk_in_port_combo_to_port(vtss_ifindex_t network_port, u8 clk_in_port, u32 *v);

/* Initialize module */
mesa_rc synce_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_SYNCE_H_ */

