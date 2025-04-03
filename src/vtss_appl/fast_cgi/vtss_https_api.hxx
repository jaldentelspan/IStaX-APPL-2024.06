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

#ifndef _VTSS_HTTPS_API_HXX_
#define _VTSS_HTTPS_API_HXX_

/* for public APIs */
#include "vtss/appl/https.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * If HTTPS management supported HTTPS automatic redirect function.
 * It will automatic redirect web browser to HTTPS during HTTPS mode enabled.
 */
#define HTTPS_MGMT_SUPPORTED_REDIRECT       1

/**
 * HTTPS management enabled/disabled
 */
#define HTTPS_MGMT_ENABLED       (1)    /**< Enable option  */
#define HTTPS_MGMT_DISABLED      (0)    /**< Disable option */

/**
 * HTTPS management default setting
 */
#define HTTPS_MGMT_DEF_MODE                     HTTPS_MGMT_DISABLED
#define HTTPS_MGMT_DEF_REDIRECT_MODE            HTTPS_MGMT_DISABLED
#define HTTPS_MGMT_DEF_ACTIVE_SESS_TIMEOUT      0   /* 0: disable active session timeout */
#define HTTPS_MGMT_DEF_ABSOLUTE_SESS_TIMEOUT    0   /* 0: disable absolute session timeout */

/**
 * Maximum allowed length
 * For the self-signed certificate, certificate and private key are stored in two separated memory space.
 * For the uploading PEM file, it is stored on one memory space since the file contains the certificate and private key together.
 */
#define HTTPS_MGMT_MAX_PKEY_LEN             (2048)  /**< Maximum allowed private key text string length (2048bits) */
#define HTTPS_MGMT_MAX_PASS_PHRASE_LEN      (64)    /**< Maximum allowed pass phrase length  */
#define HTTPS_MGMT_MAX_DH_PARAMETERS_LEN    (512)   /**< Maximum allowed DH key text string length (2048bits) */
#define HTTPS_MGMT_MAX_CERT_LEN             (HTTPS_MGMT_MAX_PKEY_LEN * 2 + HTTPS_MGMT_MAX_DH_PARAMETERS_LEN)  /**< Maximum allowed Certificate key text string length */

/**
 * Minimum/Maximum allowed timeout value (seconds)
 */
#define HTTPS_MGMT_MIN_SOFT_TIMEOUT     (0)     // The value 0 means disable active session timeout
#define HTTPS_MGMT_MAX_SOFT_TIMEOUT     (3600)  // 60 minutes
#define HTTPS_MGMT_MIN_HARD_TIMEOUT     (0)     // The value 0 means disable absolute session timeout
#define HTTPS_MGMT_MAX_HARD_TIMEOUT     (10080) // 168 hours

/**
 * \brief API Error Return Codes (mesa_rc)
 */
typedef enum {
    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_HTTPS), /**< Operation is only allowed on the primary switch. */
    HTTPS_ERROR_INV_PARAM,                                                         /**< Invalid parameter.                              */
    HTTPS_ERROR_GET_CERT_INFO,                                                     /**< Illegal get certificate information */
    HTTPS_ERROR_MUST_BE_DISABLED_MODE,                                             /**< Operation is only allowed under HTTPS mode disalbed. */
    HTTPS_ERROR_HTTPS_CORE_START,                                                  /**< HTTPS core initial failed. */
    HTTPS_ERROR_INV_CERT,                                                          /**< Invalid Certificate. */
    HTTPS_ERROR_INV_DH_PARAM,                                                      /**< Invalid DH parameter. */
    HTTPS_ERROR_INTERNAL_RESOURCE,                                                 /**< Out of internal resource. */
    HTTPS_ERROR_INV_URL,                                                           /**< Invalid URL parameter. */
    HTTPS_ERROR_UPLOAD_CERT,                                                       /**< Upload certificate failure. */
    HTTPS_ERROR_CERT_TOO_BIG,                                                      /**< Certificate PEM file size too big. */
    HTTPS_ERROR_CERT_NOT_EXISTING,                                                 /**< Certificate is not existing. */
    HTTPS_ERROR_GEN_HOSTKEY,                                                       /**< Generate host key failed. */
    HTTPS_ERROR_GEN_CA                                                             /**< Generate certificate failed. */

} https_error_t;

/**
 * \brief HTTPS Certificate status
 */
typedef enum {
    HTTPS_CERT_PRESENT,                                                 /**< Certifiaction is presented. */
    HTTPS_CERT_NOT_PRESENT,                                             /**< Certifiaction is not presented.*/
    HTTPS_CERT_IS_GENERATING,                                           /**  Certifiation is being generated  */
    HTTPS_CERT_ERROR                                                    /**  Internal error  */
} https_cert_status_t;

/**
 * \brief HTTPS configuration.
 */
typedef struct https_conf {
    BOOL mode;                                                          /**< HTTPS global mode setting.                                              */
    BOOL redirect;                                                      /**< Automatic Redirect HTTP to HTTPS during HTTPS mode enabled              */
    BOOL self_signed_cert;                                              /**< The certificate is attested to by the switch itself                     */
    BOOL prevent_csrf;                                                  /**< Security check - Prevent CSRF                                           */
    long active_sess_timeout;                                           /**< The inactivity timeout for HTTPS sessions if there is no user activity  */
    long absolute_sess_timeout;                                         /**< The hard timeout for HTTPS sessions, regardless of recent user activity */
    char server_cert[HTTPS_MGMT_MAX_CERT_LEN + 1];                      /**< Server certificate                                                      */
    char server_pkey[HTTPS_MGMT_MAX_PKEY_LEN + 1];                      /**< Server private key                                                      */
    char server_pass_phrase[HTTPS_MGMT_MAX_PASS_PHRASE_LEN + 1];        /**< Privary key pass phrase                                                 */
    char server_dh_parameters[HTTPS_MGMT_MAX_DH_PARAMETERS_LEN + 1];    /**< DH parameters, that provide algorithms for encrypting the key exchanges */
    // Constructor: mode <- TRUE, always enable HTTPS; redirect <- FALSE, disable redirect by default
    https_conf() : mode(HTTPS_MGMT_DEF_MODE), redirect(HTTPS_MGMT_DEF_REDIRECT_MODE)
    {}
} https_conf_t;

/**
 * \brief HTTPS statistics.
 */
typedef struct {
    int sess_accept;                /**< SSL new accept - started           */
    int sess_accept_renegotiate;    /**< SSL reneg - requested              */
    int sess_accept_good;           /**< SSL accept/reneg - finished        */
    int sess_miss;                  /**< session lookup misses              */
    int sess_timeout;               /**< reuse attempt on timeouted session */
    int sess_cache_full;            /**< session removed due to full cache  */
    int sess_hit;                   /**< session reuse actually done        */
    int sess_cb_hit;                /**< session-id that was not
                                     * in the cache was
                                     * passed back via the callback.  This
                                     * indicates that the application is
                                     * supplying session-id's from other
                                     * processes - spooky :-)               */
} https_stats_t;

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the HTTPS API functions.
  *
  * \param rc [IN]: Error code that must be in the HTTPS_xxx range.
  */
const char *https_error_txt(https_error_t rc);

/**
  * \brief Retrieve an status string based on a status enum
  *        from one of the HTTPS API functions.
  *
  * \param rc [IN]: status enum.
  */
const char *https_cert_status_txt(https_cert_status_t status);

/**
  * \brief Get the global HTTPS configuration.
  * Notice the https configuration take about 3K bytes, it may occurs thread
  * stacksize overflow, use allocate the memory before call the API.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc https_mgmt_conf_get(https_conf_t *const glbl_cfg);

/**
  * \brief Set the global HTTPS configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc https_mgmt_conf_set(const https_conf_t *const glbl_cfg);

/**
  * \brief Get the HTTPS certificate information.
  *
  * \param cert_info   [OUT]: The length of certificate information
  *                           should be at least 2048.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    HTTPS_ERROR_GET_CERT_INFO if get fail.\n
  */
mesa_rc https_mgmt_cert_info(char *cert_info);

/**
  * \brief Get the HTTPS certificate information.
  *
  * \param cert_info [OUT]: The certificate information.
  * \param buf_len   [IN]:  The output buffer length.
   * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    HTTPS_ERROR_GET_CERT_INFO if get fail.\n
  */
mesa_rc https_mgmt_cert_info_get(char *cert_info, size_t buf_len);

/**
  * \brief Get the HTTPS cipher information.
  *
  * \param ciphers_info [OUT]: The length of session information
  *                            should be at least 2048.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc https_mgmt_ciphers_info(char *ciphers_info, u32 ciphers_info_len);

/**
  * \brief Get the HTTPS session information.
  *
  * \param sess_info   [OUT]: The length of session information
  *                           should be at least 2048.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc https_mgmt_sess_info(char *sess_info, u32 sess_info_len);

/**
  * \brief Get the HTTPS statistics.
  *
  * \param counter [OUT]: Pointer to structure that receives
  *                       the current statistics.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc https_mgmt_counter_get(https_stats_t *stats);

/**
  * \brief Delete the HTTPS certificate.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    HTTPS_ERROR_MUST_BE_DISABLED_MODE if called on HTTPS enabled mode.\n
  */
mesa_rc https_mgmt_cert_del(void);

/**
  * \brief Generate a new self-signed RSA certificate.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc https_mgmt_cert_gen(void);

/**
  * \brief Update the HTTPS certificate.
  *        Before calling this API, https_mgmt_cert_del() must be called first
  *        Or disable HTTPS mode first.
  *
  * \param cfg [IN]: Pointer to structure that contains the
  *                  global configuration to apply to the
  *                  voice VLAN module.
  * \return
  *    VTSS_RC_OK on success.\n
  *    HTTPS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    HTTPS_ERROR_INV_CERT if Certificate is invaled.\n
  *    HTTPS_ERROR_INV_DH_PARAM if DH parameters is invaled.\n
  */
mesa_rc https_mgmt_cert_update(https_conf_t *cfg);

/**
  * \brief Initialize the HTTPS module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc https_init(vtss_init_data_t *data);

/**
  * \brief Get the HTTPS default configuration.
  *
  * \param glbl_cfg [IN_OUT]: Pointer to structure that contains the
  *                           configuration to get the default setting.
  *
  * \return
  *   Nothing.
  */
void https_conf_mgmt_get_default(https_conf_t *conf);

/**
  * \brief Determine if HTTPS configuration has changed.
  *
  * \param old [IN]: Pointer to structure that contains the
  *                  old configuration.
  * \param new [IN]: Pointer to structure that contains the
  *                  new configuration.
  *
  * \return
  *   0: No change.\n
  *   none zero: Configuration changed.\n
  */
int https_mgmt_conf_changed(const https_conf_t *const old, const https_conf_t *const new_);

/**
  * \brief Upload HTTPS certificate from URL
  *
  * \param url         [IN]: Pointer to the URL where stored the certificate PEM file.
  * \param pass_phrase [IN]: Pointer to the sting of privary key pass phrase.
  *
  * \return
  *    HTTPS_ERROR_MUST_BE_DISABLED_MODE - Current HTTPS mode is enabled, this operation can not be processed.\n
  *    HTTPS_ERROR_INTERNAL_RESOURCE - Dynamic memory allocated failure.\n
  *    HTTPS_ERROR_INV_URL - Invalid URL.\n
  *    HTTPS_ERROR_UPLOAD_CERT - Upload process failure.\n
  *    HTTPS_ERROR_INV_CERT - Invalid certificate format.\n
  *    HTTPS_ERROR_CERT_TOO_BIG - The certificate PEM file size too big.
  */
mesa_rc https_mgmt_cert_upload(const char *url, const char *pass_phrase);

/**
  * \brief Get the HTTPS certificate status.
  *
  * \return
  *  HTTPS_CERT_PRESENT         -     Certifiaction is presented.
  *  HTTPS_CERT_NOT_PRESENT     -     Certifiaction is not presented.
  *  HTTPS_CERT_IS_GENERATING   -     Certifiation is being generated.
  *  HTTPS_CERT_ERROR           -     Internal Error.
  */
https_cert_status_t https_mgmt_cert_status(void);

/**
  * \brief Save HTTPS certificate.
  *
  * \return
  *   Nothing.
  */
void https_mgmt_cert_save(void);

/**
  * \brief Check the HTTPS certificate/key is valid or not.
  *
  *
  * \param server_cert [IN]: Pointer to the sting of certificate.
  * \param server_key  [IN]: Pointer to the sting of privary key.
  * \param pass_phrase [IN]: Pointer to the sting of privary key pass phrase.
  *
  * \return
  * TRUE for valid certificate, otherwise FLASE.
  */
BOOL https_mgmt_cert_key_is_valid(const char *server_cert, const char *server_key, const char *pass_phrase);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_HTTPS_API_HXX_ */

