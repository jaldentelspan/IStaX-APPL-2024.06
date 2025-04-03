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
* \brief Public Access Management API
* \details This header file describes Access Management control functions and types.
*/

#ifndef _VTSS_APPL_ACCESS_MANAGEMENT_H_
#define _VTSS_APPL_ACCESS_MANAGEMENT_H_

#include <vtss/appl/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Access Management global configuration
 * The configuration is the system configuration that can enable/disable
 * the Access Management function.
 */
typedef struct {
    /**
     * \brief Global administrative mode, TRUE is to enable Access Management
     * function in the system and FALSE is to disable it.
     */
    mesa_bool_t                            mode;
} vtss_appl_access_mgmt_conf_t;

/**
 * \brief Access Management IPv4 table
 *  The configuration is the IPv4 settings used for running Access Management.
 */
typedef struct {
    /**
     * \brief vlan_id is used to denote the ID of specific VLAN interface
     * that Access Management should take effect.
     */
    mesa_vid_t                      vlan_id;

    /**
     * \brief start_address is used to denote starting IPv4 address of the range
     * that Access Management performs checking.
     */
    mesa_ipv4_t                     start_address;

    /**
     * \brief end_address is used to denote ending IPv4 address of the range
     * that Access Management performs checking.
     */
    mesa_ipv4_t                     end_address;

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    /**
     * \brief web_services is used to enable/disable HTTP and HTTPS traffic.
     * At least one of web_services/snmp_services/telnet_services has to be
     * enabled for a specific access index in Access Management IPv4 table.
     */
    mesa_bool_t                            web_services;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */

#ifdef VTSS_SW_OPTION_SNMP
    /**
     * \brief web_services is used to enable/disable SNMP traffic.
     * At least one of web_services/snmp_services/telnet_services has to be
     * enabled for a specific access index in Access Management IPv4 table.
     */
    mesa_bool_t                            snmp_services;
#endif /* VTSS_SW_OPTION_SNMP */

    /**
     * \brief web_services is used to enable/disable TELNET and SSH traffic.
     * At least one of web_services/snmp_services/telnet_services has to be
     * enabled for a specific access index in Access Management IPv4 table.
     */
    mesa_bool_t                            telnet_services;
} vtss_appl_access_mgmt_ipv4_t;

#ifdef VTSS_SW_OPTION_IPV6
/**
 * \brief Access Management IPv6 table
 *  The configuration is the IPv6 settings used for running Access Management.
 */
typedef struct {
    /**
     * \brief vlan_id is used to denote the ID of specific VLAN interface
     * that Access Management should take effect.
     */
    mesa_vid_t                      vlan_id;

    /**
     * \brief start_address is used to denote starting IPv6 address of the range
     * that Access Management performs checking.
     */
    mesa_ipv6_t                     start_address;

    /**
     * \brief end_address is used to denote ending IPv6 address of the range
     * that Access Management performs checking.
     */
    mesa_ipv6_t                     end_address;

#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    /**
     * \brief web_services is used to enable/disable HTTP and HTTPS traffic.
     * At least one of web_services/snmp_services/telnet_services has to be
     * enabled for a specific access index in Access Management IPv6 table.
     */
    mesa_bool_t                            web_services;
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */

#ifdef VTSS_SW_OPTION_SNMP
    /**
     * \brief web_services is used to enable/disable SNMP traffic.
     * At least one of web_services/snmp_services/telnet_services has to be
     * enabled for a specific access index in Access Management IPv6 table.
     */
    mesa_bool_t                            snmp_services;
#endif /* VTSS_SW_OPTION_SNMP */

    /**
     * \brief web_services is used to enable/disable TELNET and SSH traffic.
     * At least one of web_services/snmp_services/telnet_services has to be
     * enabled for a specific access index in Access Management IPv6 table.
     */
    mesa_bool_t                            telnet_services;
} vtss_appl_access_mgmt_ipv6_t;
#endif /* VTSS_SW_OPTION_IPV6 */

/**
 * \brief Access Management statistics
 * The statistics shows the current packet counts for accessing the system
 * management function with respect to HTTP/HTTPS/SNMP/TELNET/SSH.
 */
typedef struct {
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_HTTPS) || defined(VTSS_SW_OPTION_FAST_CGI)
    uint32_t                             http_receive_cnt;   /**< Received count via HTTP */
    uint32_t                             http_permit_cnt;    /**< Allowed count via HTTP */
    uint32_t                             http_discard_cnt;   /**< Discarded count via HTTP */
    uint32_t                             https_receive_cnt;  /**< Received count via HTTPS */
    uint32_t                             https_permit_cnt;   /**< Allowed count via HTTPS */
    uint32_t                             https_discard_cnt;  /**< Discarded count via HTTPS */
#endif /* VTSS_SW_OPTION_WEB  || VTSS_SW_OPTION_HTTPS || VTSS_SW_OPTION_FAST_CGI */

#ifdef VTSS_SW_OPTION_SNMP
    uint32_t                             snmp_receive_cnt;   /**< Received count via SNMP */
    uint32_t                             snmp_permit_cnt;    /**< Allowed count via SNMP */
    uint32_t                             snmp_discard_cnt;   /**< Discarded count via SNMP */
#endif /* VTSS_SW_OPTION_SNMP */

    uint32_t                             telnet_receive_cnt; /**< Received count via TELNET */
    uint32_t                             telnet_permit_cnt;  /**< Allowed count via TELNET */
    uint32_t                             telnet_discard_cnt; /**< Discarded count via TELNET */
    uint32_t                             ssh_receive_cnt;    /**< Received count via SSH */
    uint32_t                             ssh_permit_cnt;     /**< Allowed count via SSH */
    uint32_t                             ssh_discard_cnt;    /**< Discarded count via SSH */
} vtss_appl_access_mgmt_statistics_t;

/**
 * \brief Get Access Management Parameters
 *
 * To read current system parameters in Access Management.
 *
 * \param conf      [OUT]    The Access Management system configuration data.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_system_config_get(
    vtss_appl_access_mgmt_conf_t        *const conf
);

/**
 * \brief Set Access Management Parameters
 *
 * To modify current system parameters in Access Management.
 *
 * \param conf      [IN]     The Access Management system configuration data.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_system_config_set(
    const vtss_appl_access_mgmt_conf_t  *const conf
);

/**
 * \brief Access Management Control ACTION
 *
 * Action flag to denote clearing Access Management statistics.
 * This flag is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means we should take action for clearing all the counters
 * in Access Management statistics table.
 *
 * \param clr_flag  [IN]    Clear Access Management statistics action to be taken.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_control_statistics_clr(
    const mesa_bool_t                          *clr_flag
);

/**
 * \brief Iterator for retrieving Access Management IPv4 table key/index
 *
 * To walk access index of the IPv4 table in Access Management.
 *
 * \param prev      [IN]    Access index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_itr(
    const uint32_t                           *const prev,
    uint32_t                                 *const next
);

/**
 * \brief Get Access Management specific access index configuration
 *
 * To read configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv4_conf [OUT]   The current configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_get(
    uint32_t                                 access_id,
    vtss_appl_access_mgmt_ipv4_t        *const ipv4_conf
);

/**
 * \brief Set Access Management specific access index configuration
 *
 * To modify configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv4_conf [IN]    The revised configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_set(
    uint32_t                                 access_id,
    const vtss_appl_access_mgmt_ipv4_t  *const ipv4_conf
);

/**
 * \brief Delete Access Management specific access index configuration
 *
 * To delete configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_del(
    uint32_t                                 access_id
);

/**
 * \brief Add Access Management specific access index configuration
 *
 * To add configuration of the specific access index in Access Management IPv4 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv4_conf [IN]    The new configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv4_table_add(
    uint32_t                                 access_id,
    const vtss_appl_access_mgmt_ipv4_t  *const ipv4_conf
);

#ifdef VTSS_SW_OPTION_IPV6
/**
 * \brief Iterator for retrieving Access Management IPv6 table key/index
 *
 * To walk access index of the IPv6 table in Access Management.
 *
 * \param prev      [IN]    Access index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_itr(
    const uint32_t                           *const prev,
    uint32_t                                 *const next
);

/**
 * \brief Get Access Management specific access index configuration
 *
 * To read configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv6_conf [OUT]   The current configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_get(
    uint32_t                                 access_id,
    vtss_appl_access_mgmt_ipv6_t        *const ipv6_conf
);

/**
 * \brief Set Access Management specific access index configuration
 *
 * To modify configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv6_conf [IN]    The revised configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_set(
    uint32_t                                 access_id,
    const vtss_appl_access_mgmt_ipv6_t  *const ipv6_conf
);

/**
 * \brief Delete Access Management specific access index configuration
 *
 * To delete configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_del(
    uint32_t                                 access_id
);

/**
 * \brief Add Access Management specific access index configuration
 *
 * To add configuration of the specific access index in Access Management IPv6 table.
 *
 * \param access_id [IN]    (key) Access index.  Accepted values are in the range [1; 16].
 * \param ipv6_conf [IN]    The new configuration of the access index.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_ipv6_table_add(
    uint32_t                                 access_id,
    const vtss_appl_access_mgmt_ipv6_t  *const ipv6_conf
);
#endif /* VTSS_SW_OPTION_IPV6 */

/**
 * \brief Get current Access Management statistics
 *
 * To read the counters of received/allowed/discarded HTTP/HTTPS/SNMP/TELNET/SSH frames.
 *
 * \param pkt_cntr  [OUT]   The current packet counters for HTTP/HTTPS/SNMP/TELNET/SSH.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc
vtss_appl_access_mgmt_statistics_get(
    vtss_appl_access_mgmt_statistics_t  *const pkt_cntr
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_ACCESS_MANAGEMENT_H_ */
