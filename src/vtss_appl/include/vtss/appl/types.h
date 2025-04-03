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

/**
 * \file
 * \brief Public Types
 * \details This header file describes public types
 */

#ifndef _VTSS_APPL_TYPES_H_
#define _VTSS_APPL_TYPES_H_

#include <string.h> /* For size_t  */
#include <microchip/ethernet/switch/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Represents a sequence of physical ports in stacked or non-stacked
 * units.
 *
 *  This is the underlying storage type of the vtss::PortList which is found in
 *  vtss/appl/types.hxx
 */
typedef struct {
    unsigned char data[128]; /**< Internal data, do not access directly */
} vtss_port_list_stackable_t;

/**
 * \brief A bit-field representing a vlan list. The least significant bit of
 * data[0] represents vlan 1, the most significant bit of data[0] represents
 * vlan 8. The least significant bit of data[1] represents vlan 9 etc.
 */
typedef struct {
    uint8_t data[512]; /**< Data representing 4K VLANs as described in the enclosing structure's description */
} vtss_vlan_list_t;

/**
 *  \brief DNS domain name buffer.
 */
typedef struct {
    /** The buffer containing the host name. The buffer is used as a c-string,
     * and must be null terminated */
    char name[255];
} vtss_domain_name_t;

/**
 *  \brief Internet address types.
 */
typedef enum {
    /** Internet address type: none */
    VTSS_INET_ADDRESS_TYPE_NONE = 0,

    /** Internet address type: IPv4 */
    VTSS_INET_ADDRESS_TYPE_IPV4 = 1,

    /** Internet address type: IPv6 */
    VTSS_INET_ADDRESS_TYPE_IPV6 = 2,

    /** Internet address type: DNS domain name */
    VTSS_INET_ADDRESS_TYPE_DNS = 3
} vtss_inet_address_type_t;

/**
 *  \brief Data structure for representing Internet address.
 *
 *  The Internet address can be IPv4 address, IPv6 address, or DNS domain name.
 */
typedef struct {
    vtss_inet_address_type_t type; /*!< Internet address type */

    union {
        mesa_ipv4_t ipv4;               /*!< IPv4 address */
        mesa_ipv6_t ipv6;               /*!< IPv6 address */
        vtss_domain_name_t domain_name; /*!< DNS domain name */
    } address; /**< Address, which can be either an IPv4, IPv6, or a DNS domain name */
} vtss_inet_address_t;

/* ================================================================= *
 *  Integer types
 * ================================================================= */
typedef unsigned int        uint;      /**< Short for unsigned integer   */
typedef unsigned short      ushort;    /**< Short for unsigned short     */
typedef unsigned char       uchar;     /**< Short for unsigned character */
typedef signed char         schar;     /**< Short for signed integer     */
typedef long long           longlong;  /**< Short for long long          */
typedef unsigned long long  ulonglong; /**< Short for unsigned long long */

/* ================================================================= *
 *  User Switch IDs (USIDs), used by management modules (CLI, Web)
 * ================================================================= */
#define VTSS_USID_START  1                                                      /**< The first User Switch ID                                                    */
#define VTSS_USID_CNT    VTSS_ISID_CNT                                          /**< The number of User Switch IDs                                               */
#define VTSS_USID_END    (VTSS_USID_START + VTSS_USID_CNT)                      /**< The last+1 User Switch ID                                                   */
#define VTSS_USID_ALL    0                                                      /**< Special value for selecting all switches. Only valid in selected contexts!  */
#define VTSS_USID_LEGAL(usid) (usid >= VTSS_USID_START && usid < VTSS_USID_END) /**< Macro returning TRUE if \p usid is a valid User Switch ID, FALSE otherwise. */

typedef uint32_t vtss_usid_t; /**< User switch IDs are switch IDs used by management modules, like CLI and Web, as opposed to vtss_isid_t which is for internal use. There is a mapping between USIDs and ISIDs. */

/* ================================================================= *
 *  User Port IDs, used by management modules (CLI, Web)
 * ================================================================= */
typedef mesa_port_no_t vtss_uport_no_t; /**< User ports numbers (uport_no) are port numbers as presented in CLI and Web */

/* ================================================================= *
 *  Internal Switch IDs (ISIDs)
 * ================================================================= */
#define VTSS_ISID_START   1                                                     /**< The first Internal Switch ID                                                    */
#define VTSS_ISID_CNT     1                                                     /**< The maximum number of Internal Switch IDs in a standalone configuration         */
#define VTSS_ISID_END     (VTSS_ISID_START + VTSS_ISID_CNT)                     /**< The last+1 Internal Switch ID                                                   */
#define VTSS_ISID_LOCAL   0                                                     /**< Special value for local switch. Only valid in selected contexts!                */
#define VTSS_ISID_UNKNOWN 0xff                                                  /**< Special value only used in selected contexts!                                   */
#define VTSS_ISID_GLOBAL VTSS_ISID_END                                          /**< Special value for all switches. Only valid in selected contexts!                */
#define VTSS_ISID_LEGAL(isid) (isid >= VTSS_ISID_START && isid < VTSS_ISID_END) /**< Macro returning TRUE if \p isid is a valid Internal Switch ID, FALSE otherwise. */

typedef uint32_t vtss_isid_t; /**< Internal switch IDs are switch IDs used internally by the various modules, as opposed to vtss_usid_t which is for exposed management interface use. There is a mapping between USIDs and ISIDs. */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _VTSS_APPL_TYPES_H_

