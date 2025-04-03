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

/**
 * \file
 * \brief Public HTTPS API
 * \details This header file describes HTTPS control functions and types.
 */

#ifndef _VTSS_APPL_HTTPS_H_
#define _VTSS_APPL_HTTPS_H_

#include <vtss/appl/types.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

#define VTSS_APPL_HTTPS_CERT_URL_LEN            2048  /**< Maximum length of https certificate url */
#define VTSS_APPL_HTTPS_CERT_PASS_PHRASE_LEN    2048  /**< Maximum length of https certificate private key pass phrase */

/**
 * \brief HTTPS global configuration
 *  The configuration is the system configuration that can enable/disable
 *  the HTTP Secure function and also can enable/disable to automatically
 *  redirect HTTP to HTTPS.
 */
typedef struct {
    /**
     * \brief Global config mode, TRUE is to enable HTTPS function
     * in the system and FALSE is to disable it.
     */
    mesa_bool_t    mode;

    /**
     * \brief The mode is to enable/disable the automatic redirection from HTTP
     * to HTTPS. TRUE is to enable the redirection and FALSE is to disable
     * the redirection.
     */
    mesa_bool_t    redirect_to_https;
} vtss_appl_https_param_t;

/**
 * \brief HTTPS certificate configuration
 * This configuration is used to generate the https certificate
 */
typedef struct {
    char url[VTSS_APPL_HTTPS_CERT_URL_LEN];                    /**< Pointer to the URL where stored the certificate PEM file */
    char pass_phrase[VTSS_APPL_HTTPS_CERT_PASS_PHRASE_LEN];    /**< Pointer to the string of private key pass phrase */
} vtss_appl_https_cert_t;

/**
 * \brief Delete https certificate
 */
typedef struct {
    mesa_bool_t del;  /**< Delete the https certificate */
} vtss_appl_https_cert_del_t;

/**
 * \brief Generate https certificate
 */
typedef struct {
    mesa_bool_t gen;  /**< Generate the https certificate */
} vtss_appl_https_cert_gen_t;

/**
 * \brief Get HTTPS system parameters
 *
 * To read current system parameters in HTTPS.
 *
 * \param param [OUT] The HTTPS system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_https_system_config_get(
    vtss_appl_https_param_t          *const param
);

/**
 * \brief Set HTTPS system parameters
 *
 * To modify current system parameters in HTTPS.
 *
 * \param param [IN] The HTTPS system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_https_system_config_set(
    const vtss_appl_https_param_t     *const param
);

/**
 * \brief Upload HTTPS certificate from URL
 *
 * This is a wrapper function that is essentially calling
 * https_mgmt_cert_upload()
 *
 * \param cert [IN]: HTTPS certificate parameter for upload
 *
 * \return VTSS_RC_OK if the operation succeed
 */
mesa_rc vtss_appl_https_cert_upload(
    const vtss_appl_https_cert_t *const cert
);

/**
 * \brief Delete HTTPS certificat
 *
 * This is a wrapper function that is essentially calling
 * https_mgmt_cert_del()
 *
 * \param del [IN] HTTPS certificate parameter for delete
 *
 * \return VTSS_RC_OK if the operation succeed
 */
mesa_rc vtss_appl_https_cert_delete(
    const vtss_appl_https_cert_del_t *const del
);

/**
 * \brief Dummy function that delete HTTPS certificate
 *
 * \param del [IN] HTTPS certificate parameter for delete
 *
 * \return VTSS_RC_OK always return VTSS_RC_OK
 */
mesa_rc vtss_appl_https_cert_delete_dummy(
    vtss_appl_https_cert_del_t *const del);

/**
 * \brief Generate HTTPS certificate
 *
 * \param gen [IN] HTTPS certificate parameter for generate new certificate
 *
 * \return VTSS_RC_OK if the operation succeed
 */
mesa_rc vtss_appl_https_cert_generate(
    const vtss_appl_https_cert_gen_t *const gen
                                      );
/**
 * \brief Dummy function that generate HTTPS certificate
 *
 * \param gen [IN] HTTPS certificate parameter for generate new certificate
 *
 * \return VTSS_RC_OK always return VTSS_OL
 */
mesa_rc vtss_appl_https_cert_generate_dummy(
    vtss_appl_https_cert_gen_t *const gen
                                            );

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_HTTPS_H_ */
