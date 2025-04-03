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

#ifndef _VTSS_HOSTADDR_H_
#define _VTSS_HOSTADDR_H_

#include <main_types.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Address/Name information of the host entry. */
typedef struct {
    /** Name of the host. */
    char                h_name[256];

    /** Address of the host. */
    mesa_ip_type_t      type;       /* Type of mapping address */
    union {
        struct {
            u16         family;     /* AF_INET (sa_family_t) */
            u16         port;       /* Transport layer port # (in_port_t) */
            mesa_ipv4_t address;    /* IP4 address */
        } ipv4;

        struct {
            u16         family;     /* AF_INET6 (sa_family_t) */
            u16         port;       /* Transport layer port # (in_port_t) */
            u32         flowinfo;   /* IP6 flow information */
            mesa_ipv6_t address;    /* IP6 address */
            u32         scope_id;   /* Scope zone index */
        } ipv6;
    }  h_address;
} vtss_hostent_t;

/****************************************************************************
 * Public functions
*****************************************************************************/

/*!
 * \brief Get network host entry
 *
 * \param hostname    [IN]  name of the host
 * \param host        [OUT] a structure to contain an internet address
 *
 * \return Error code.
 */
    mesa_rc vtss_getaddrinfo(const char* hostname, struct sockaddr_in* host, int flags, int family, char* errcode);

/*!
 * \brief Prints the error message associated with the current value of h_errno on stderr.
 *
 * \param s             [OUT] The error message associated with the current value of h_errno on stderr.
 */
void vtss_herror(const char *str);

/*!
 * \brief Prints the error message w.r.t the input error code (h_errno).
 *
 * \param eno           [IN]  Error code.
 *
 * \return Error string.
 */
const char *vtss_hstrerror(int eno);

/*!
 * \brief Convert host entry to sockaddr_in or sockaddr_in6
 *
 * \param hostent       [IN]  Host entry.
 * \param sockadr       [OUT] A structure to contain the address information.
 *
 * \return Error code.
 */
mesa_rc vtss_hostent_to_sockaddr(
    const vtss_hostent_t    *const hostent,
    void                    *const sockadr
);

/*!
 * \brief Get network host entry by given name.
 *
 * \param hostname      [IN]  Name string of the host.
 * \param type          [IN]  Address family of the host.
 * \param host          [OUT] A structure to contain the address/name information of the host.
 *
 * \return Error code.
 */
mesa_rc vtss_gethostbyname(
    const char          *const hostname,
    int                 type,
    vtss_hostent_t      *const host
);

/*!
 * \brief Get network host entry by given address.
 *
 * \param hostaddr      [IN]  Address string of the host.
 * \param type          [IN]  Address family of the host.
 * \param host          [OUT] A structure to contain the address/name information of the host.
 *
 * \return Error code.
 */
mesa_rc vtss_gethostbyaddr(
    const void          *const hostaddr,
    int                 type,
    vtss_hostent_t      *const host
);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_HOSTADDR_H_ */

