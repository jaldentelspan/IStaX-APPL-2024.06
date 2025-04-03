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
 * \brief Public Alarm API
 * \details This header file describes alarm control functions and types
 */

#include <vtss/basics/api_types.h>

#ifndef _VTSS_APPL_ALARM_H_
#define _VTSS_APPL_ALARM_H_

/** The maximum size of a dotted alarm name */
#define ALARM_NAME_SIZE 100

/** The maximum size of an alarm expression */
#define ALARM_EXPRESSION_SIZE 1024

/** The maximum size of a type as string */
#define ALARM_SOURCE_TYPE_SIZE 64

/** The maximum size of a type as string */
#define ALARM_SOURCE_ENUM_VALUES_SIZE 256

/** \brief The name of the alarm. */
typedef struct {
    /** The name of the alarm, e.g. alarm.port.link */
    char alarm_name[ALARM_NAME_SIZE];
} vtss_appl_alarm_name_t;

/** \brief The expression defining the alarm */
typedef struct {
    /** The expression defining the alarm. */
    char alarm_expression[ALARM_EXPRESSION_SIZE];
} vtss_appl_alarm_expression_t;

/** \brief The alarm suppression configuration*/
typedef struct {
    /** Suppression of the alarm. */
    mesa_bool_t suppress;
} vtss_appl_alarm_suppression_t;

/** \brief The alarm status*/
typedef struct {
    /** Suppression of the alarm. */
    mesa_bool_t suppressed;

    /** Status of the alarm. */
    mesa_bool_t active;
    /** Resulting status of the alarm. */
    mesa_bool_t exposed_active;
} vtss_appl_alarm_status_t;

/** \brief The alarm source entry info */
typedef struct {
    /** The name of the alarm source, e.g. alarm.port.link */
    char alarm_name[ALARM_NAME_SIZE];

    /** Type of alarm source */
    char type[ALARM_SOURCE_TYPE_SIZE];

    /** If the alarm source type is an enumerated value, this is a listing of
      * the allowed enumerated values. */
    char enum_values[ALARM_SOURCE_ENUM_VALUES_SIZE];
} vtss_appl_alarm_source_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Iterators -------------------------------------------------------------- */

/**
 * \brief Iterate through all alarms.
 * \param in [IN]   Pointer to current alarm index. Provide a null pointer
 *                  to get the first alarm.
 * \param out [OUT] Next alarm index (relative to the value provided in
 *                  'in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" alarm index exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_alarm_conf_itr(const vtss_appl_alarm_name_t *const in,
                                 vtss_appl_alarm_name_t *const out);

/* Alarm functions ----------------------------------------------------- */

/**
 * \brief Get configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param conf [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_conf_get(const vtss_appl_alarm_name_t *const nm,
                                 vtss_appl_alarm_expression_t *conf);

/**
 * \brief Create configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param conf [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_conf_add(const vtss_appl_alarm_name_t *const nm,
                                 const vtss_appl_alarm_expression_t *const conf);

/**
 * \brief Delete configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_conf_del(const vtss_appl_alarm_name_t *const nm);

/**
 * \brief Iterate through all alarm status.
 * \param in [IN]   Pointer to current alarm index. Provide a null pointer
 *                  to get the first alarm node or leaf.
 * \param out [OUT] Next alarm index (relative to the value provided in
 *                  'in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" alarm index exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_alarm_status_itr(const vtss_appl_alarm_name_t *const in,
                                   vtss_appl_alarm_name_t *const out);
/**
 * \brief Get suppression configuration for a specific alarm node or leaf
 * \param nm [IN] Name of the alarm node or leaf
 * \param supp [OUT] The current configuration of suppression
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_suppress_get(const vtss_appl_alarm_name_t  *const nm,
                                     vtss_appl_alarm_suppression_t *const supp);

/**
 * \brief Set suppression configuration for a specific alarm node or leaf
 * \param nm [IN] Name of the alarm node or leaf
 * \param supp [IN] The configuration of suppression
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_suppress_set(const vtss_appl_alarm_name_t  *const nm,
                                     const vtss_appl_alarm_suppression_t *const supp);

/**
 * \brief Get configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param status [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_alarm_status_get(const vtss_appl_alarm_name_t *const nm,
                                   vtss_appl_alarm_status_t     *status);


/**
 * \brief Iterate through all alarm_sources.
 * \param in  [IN] pointer to index in alarm_source vector. Provide null pointer to
 *                 get the first source in the list
 * \param out [OUT] Next alarm index (relative to the value provided in
 *                  'in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" alarm source exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_alarm_sources_itr(const int *const in, int *const out);

/**
 * \brief Get specific alarm source
 * \param in [IN]  Pointer to index in alarm_source vector.
 * \param as [OUT] Pointer to structure where the source can be stored.
 * \return Error code. VTSS_RC_OK means that the value in str is valid,
 *                     VTSS_RC_ERROR means that no source was found.
 */
mesa_rc vtss_appl_alarm_sources_get(const int *const in,
                                    vtss_appl_alarm_source_t *as);
#ifdef __cplusplus
}
#endif

#endif  // _VTSS_APPL_ALARM_H_
