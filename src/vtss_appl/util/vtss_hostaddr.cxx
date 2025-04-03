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

#include "vtss_hostaddr.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

mesa_rc vtss_getaddrinfo(const char* hostname, struct sockaddr_in* host, int flags, int family, char* errcode) {
    int rc;
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    hints.ai_flags = flags;
    hints.ai_family = family;
    hints.ai_socktype = SOCK_DGRAM;   /* Datagram socket */
    hints.ai_protocol = 0;            /* Any protocol */

    rc = getaddrinfo(hostname, NULL, &hints, &result);
    if (rc!=0) {
        strcpy(errcode, gai_strerror(rc));
        return VTSS_RC_ERROR;
    } else {
        host->sin_addr.s_addr = *((uint32_t*) & (((sockaddr_in*)result->ai_addr)->sin_addr));
    }
    freeaddrinfo(result);
    return VTSS_RC_OK;
}

/*!
 * \brief Prints the error message associated with the current value of h_errno on stderr.
 *
 * \param s             [OUT] The error message associated with the current value of h_errno on stderr.
 */
void vtss_herror(const char *str)
{
    if (h_errno) {
        str = gai_strerror(h_errno);
    } else {
        str = "Failed to resolve IP address";
    }
}

/*!
 * \brief Prints the error message w.r.t the input error code (h_errno).
 *
 * \param eno           [IN]  Error code.
 *
 * \return Error string.
 */
const char *vtss_hstrerror(int eno)
{
    return gai_strerror(eno);
}

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
)
{
    if (!hostent || !sockadr ||
        hostent->type == MESA_IP_TYPE_NONE) {
        return VTSS_RC_ERROR;
    }

    if (hostent->type == MESA_IP_TYPE_IPV4) {
        struct sockaddr_in  *adr4 = (struct sockaddr_in *)sockadr;

        memset(adr4, 0x0, sizeof(struct sockaddr_in));
        adr4->sin_family = AF_INET;
        adr4->sin_port = hostent->h_address.ipv4.port;
        adr4->sin_addr.s_addr = hostent->h_address.ipv4.address;

        return VTSS_RC_OK;
    } else if (hostent->type == MESA_IP_TYPE_IPV6) {
#ifdef VTSS_SW_OPTION_IPV6
        struct sockaddr_in6 *adr6 = (struct sockaddr_in6 *)sockadr;

        memset(adr6, 0x0, sizeof(struct sockaddr_in6));
        adr6->sin6_family = AF_INET6;
        adr6->sin6_port = hostent->h_address.ipv6.port;
        adr6->sin6_flowinfo = hostent->h_address.ipv6.flowinfo;
        memcpy(&adr6->sin6_addr.s6_addr, &hostent->h_address.ipv6.address, sizeof(struct in6_addr));
        adr6->sin6_scope_id = hostent->h_address.ipv6.scope_id;

        return VTSS_RC_OK;
#endif /* VTSS_SW_OPTION_IPV6 */
    }

    return VTSS_RC_ERROR;
}

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
)
{
    int                 rc;
    struct addrinfo     hints, *aixx, *ai = NULL;

    if (!hostname || !host) {
        return VTSS_RC_ERROR;
    }

#ifdef VTSS_SW_OPTION_IPV6
    if (type != AF_INET && type != AF_INET6) {
        return VTSS_RC_ERROR;
    }
#else
    if (type != AF_INET) {
        return VTSS_RC_ERROR;
    }
#endif /* VTSS_SW_OPTION_IPV6 */

    memset(&hints, 0x0, sizeof(hints));
    hints.ai_family = type;
    hints.ai_flags |= AI_CANONNAME;
    if ((rc = getaddrinfo(hostname, NULL, &hints, &ai)) != 0) {
        return (mesa_rc)rc;
    }

    for (aixx = ai; aixx; aixx = aixx->ai_next) {
        if (aixx->ai_family != type ||
            aixx->ai_addr == NULL) {
            continue;
        }

        memset(host, 0x0, sizeof(vtss_hostent_t));
        strncpy(host->h_name, aixx->ai_canonname, strlen(aixx->ai_canonname));
        if (type == AF_INET) {
            struct sockaddr_in  *adr4 = (struct sockaddr_in *)aixx->ai_addr;

            host->type = MESA_IP_TYPE_IPV4;
            host->h_address.ipv4.family = adr4->sin_family;
            host->h_address.ipv4.port = adr4->sin_port;
            host->h_address.ipv4.address = adr4->sin_addr.s_addr;
        } else {
#ifdef VTSS_SW_OPTION_IPV6
            struct sockaddr_in6 *adr6 = (struct sockaddr_in6 *)aixx->ai_addr;

            host->type = MESA_IP_TYPE_IPV6;
            host->h_address.ipv6.family = adr6->sin6_family;
            host->h_address.ipv6.port = adr6->sin6_port;
            host->h_address.ipv6.flowinfo = adr6->sin6_flowinfo;
            memcpy(&host->h_address.ipv6.address, &adr6->sin6_addr.s6_addr, sizeof(mesa_ipv6_t));
            host->h_address.ipv6.scope_id = adr6->sin6_scope_id;
#endif /* VTSS_SW_OPTION_IPV6 */
        }

        break;
    }

    freeaddrinfo(ai);
    if (!aixx) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

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
)
{
    int                 rc;
    socklen_t           len;
    struct sockaddr     *hst;
    char                namebuf[256];
    struct sockaddr_in  adr4;
#ifdef VTSS_SW_OPTION_IPV6
    struct sockaddr_in6 adr6;
#endif /* VTSS_SW_OPTION_IPV6 */

    if (!hostaddr || !host) {
        return VTSS_RC_ERROR;
    }

#ifdef VTSS_SW_OPTION_IPV6
    if (type != AF_INET && type != AF_INET6) {
        return VTSS_RC_ERROR;
    }
#else
    if (type != AF_INET) {
        return VTSS_RC_ERROR;
    }
#endif /* VTSS_SW_OPTION_IPV6 */

    len = 0;
    hst = NULL;
    if (type == AF_INET) {
        memset(&adr4, 0x0, sizeof(struct sockaddr_in));
        adr4.sin_family = AF_INET;
        len = sizeof(struct sockaddr_in);
        adr4.sin_addr = *(struct in_addr *)hostaddr;

        hst = (struct sockaddr *)&adr4;
    } else {
#ifdef VTSS_SW_OPTION_IPV6
        memset(&adr6, 0x0, sizeof(struct sockaddr_in6));
        adr6.sin6_family = AF_INET6;
        len = sizeof(struct sockaddr_in6);
        memcpy(&adr6.sin6_addr.s6_addr, (struct in6_addr *)hostaddr, sizeof(struct in6_addr));

        hst = (struct sockaddr *)&adr6;
#endif /* VTSS_SW_OPTION_IPV6 */
    }

    if (!hst || !len) {
        return VTSS_RC_ERROR;
    }

    if ((rc = getnameinfo(hst, len, namebuf, sizeof(namebuf), NULL, 0, NI_NUMERICHOST)) != 0) {
        return (mesa_rc)rc;
    }

    memset(host, 0x0, sizeof(vtss_hostent_t));
    strncpy(host->h_name, namebuf, strlen(namebuf));
    if (type == AF_INET) {
        host->type = MESA_IP_TYPE_IPV4;
        host->h_address.ipv4.family = AF_INET;
        host->h_address.ipv4.address = ((struct in_addr *)hostaddr)->s_addr;
    } else {
#ifdef VTSS_SW_OPTION_IPV6
        host->type = MESA_IP_TYPE_IPV6;
        host->h_address.ipv6.family = AF_INET6;
        memcpy(&host->h_address.ipv6.address, (struct in6_addr *)hostaddr, sizeof(mesa_ipv6_t));
#endif /* VTSS_SW_OPTION_IPV6 */
    }

    return VTSS_RC_OK;
}
