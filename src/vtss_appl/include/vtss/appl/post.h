/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/**
 * \file
 * \brief Public API of POST
 * \details POST is an acronym for Post On Self Telf.
 * It is run automatically on various components at poweron. The power on self
 * test (POST) is used to test the basic hardware. It includes ready-made
 * tests (e.g. BIST) embedded in hardware or ASICs such as memory tests,
 * serdes tests, internal loopback test etc.
 */

#ifndef _VTSS_APPL_POST_H_
#define _VTSS_APPL_POST_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/** Maximum number of monitor IC */
#define VTSS_APPL_POST_MONITOR_IC_MAX_CNT   2

/** Minmum ID of monitor IC */
#define VTSS_APPL_POST_MONITOR_IC_MIN_ID    1

/** Maxmum ID of monitor IC */
#define VTSS_APPL_POST_MONITOR_IC_MAX_ID    (1 + (VTSS_APPL_POST_MONITOR_IC_MAX_CNT) - (VTSS_APPL_POST_MONITOR_IC_MIN_ID))

/** Maximum string length */
#define VTSS_APPL_POST_STRING_MAX_LEN       15

/**
 * \brief Global configurations
 */
typedef struct {
    mesa_bool_t    mode; /*!< enable/disable POST function */
} vtss_appl_post_config_globals_t;

/**
 * \brief Test result
 */
typedef enum {
    VTSS_APPL_POST_TEST_RESULT_NOT_TESTED,  /*!< Not tested. */
    VTSS_APPL_POST_TEST_RESULT_PASS,        /*!< Pass the test. */
    VTSS_APPL_POST_TEST_RESULT_FAILED,      /*!< The test fails. */
} vtss_appl_post_test_result_t;

/**
 * \brief Hardware component status table
 */
typedef struct {
    vtss_appl_post_test_result_t    hw_bist;        /*!< Hareware Built-In Self-Test(BIST). */
    vtss_appl_post_test_result_t    tcam_bist_is0;  /*!< TCAM BIST on IS0. */
    vtss_appl_post_test_result_t    tcam_bist_is1;  /*!< TCAM BIST on IS1. */
    vtss_appl_post_test_result_t    tcam_bist_is2;  /*!< TCAM BIST on IS2. */
    vtss_appl_post_test_result_t    tcam_bist_es0;  /*!< TCAM BIST on ES0. */
    vtss_appl_post_test_result_t    ddr;            /*!< DDR SDRAM test. */
    vtss_appl_post_test_result_t    eeprom;         /*!< EEPROM test. */
} vtss_appl_post_status_hw_component_entry_t;

/**
 * \brief Interface status table
 */
typedef struct {
    vtss_appl_post_test_result_t    loopback;       /*!< Loopback test. */
    vtss_appl_post_test_result_t    i2c_bus_scan;   /*!< I2C bus scan test. */
} vtss_appl_post_status_interface_entry_t;

/**
 * \brief Monitor IC status table
 */
typedef struct {
    vtss_appl_post_test_result_t    i2c_bus_scan;                             /*!< I2C bus scan test.             */
    char                            v5[VTSS_APPL_POST_STRING_MAX_LEN + 1];    /*!< Voltage on 5v.                 */
    char                            v12[VTSS_APPL_POST_STRING_MAX_LEN + 1];   /*!< Voltage on 12v.                */
    char                            v2_5[VTSS_APPL_POST_STRING_MAX_LEN + 1];  /*!< Voltage on 2.5v.               */
    char                            vccp[VTSS_APPL_POST_STRING_MAX_LEN + 1];  /*!< Voltage on vccp.               */
    uint32_t                             local_temperature;                        /*!< Local temperature in Celsius.  */
    uint32_t                             remote_temperature;                       /*!< Remote temperature in Celsius. */
} vtss_appl_post_status_monitor_ic_entry_t;

/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief Get global configuration.
 *
 * \param globals [OUT] Global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_config_globals_get(
    vtss_appl_post_config_globals_t     *const globals
);

/**
 * \brief Set global configuration.
 *
 * \param globals [IN] Global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_config_globals_set(
    const vtss_appl_post_config_globals_t   *const globals
);

/**
 * \brief Iterate function of hardware component status table
 *
 * \param prev_usid [IN]  previous switch ID.
 * \param next_usid [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_status_hw_component_entry_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
);

/**
 * \brief Get hardware component status.
 *
 * \param usid  [IN]  Switch ID
 * \param entry [OUT] Hardware component status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_status_hw_component_entry_get(
    vtss_usid_t                                 usid,
    vtss_appl_post_status_hw_component_entry_t  *const entry
);

/**
 * \brief Iterate function of interface status table
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_status_interface_entry_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get port status
 *
 * \param ifindex [IN]  ifindex of port
 * \param entry   [OUT] Port status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_status_interface_entry_get(
    vtss_ifindex_t                              ifindex,
    vtss_appl_post_status_interface_entry_t     *const entry
);

/**
 * \brief Iterate function of monitor IC table
 *
 * \param prev_usid  [IN]  previous switch ID.
 * \param next_usid  [OUT] next switch ID.
 * \param prev_ic_id [IN]  ID of previous IC.
 * \param next_ic_id [OUT] ID of next IC.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_status_monitor_ic_entry_itr(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid,
    const uint32_t           *const prev_ic_id,
    uint32_t                 *const next_ic_id
);

/**
 * \brief Get IC status
 *
 * \param usid  [IN]  Switch ID
 * \param ic_id [IN]  IC ID
 * \param entry [OUT] IC status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_post_status_monitor_ic_entry_get(
    vtss_usid_t                                 usid,
    uint32_t                                         ic_id,
    vtss_appl_post_status_monitor_ic_entry_t    *const entry
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_POST_H_ */
