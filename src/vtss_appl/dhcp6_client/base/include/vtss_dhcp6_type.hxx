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

#ifndef _VTSS_DHCP6_TYPE_HXX_
#define _VTSS_DHCP6_TYPE_HXX_

#define VTSS_IPV6_PACK_STRUCT           __attribute__((packed))
#define VTSS_DHCP6_PACK_STRUCT          VTSS_IPV6_PACK_STRUCT

#define VTSS_IPV6_ETHER_LENGTH          (6 + 6 + 2)
#define VTSS_IPV6_HEADER_VERSION        6
#define VTSS_IPV6_HEADER_NXTHDR_UDP     17
#define VTSS_DHCP6_CLIENT_UDP_PORT      546
#define VTSS_DHCP6_SERVER_UDP_PORT      547
#define VTSS_DHCP6_LIFETIME_INFINITY    0xFFFFFFFF
#define VTSS_DHCP6_INFINITE_LT(x)       ((x) == VTSS_DHCP6_LIFETIME_INFINITY)
#define VTSS_DHCP6_MAX_LIFETIME         (0xFFFFFFFFFFFFFFFFLLU - VTSS_OS_MSEC2TICK(1000LLU))

namespace vtss
{
namespace dhcp6
{
#define DHCP6_LINKSCOPE_RELAY_AGENTS_AND_SERVERS_INIT   \
    {{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
       0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02 }}

#define DHCP6_SITESCOPE_ALL_DHCP_SERVERS_INIT           \
    {{ 0xff, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
       0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03 }}

#define DHCP6_UNSPECIFIED_ADDRESS_INIT                  \
    {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  \
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}

const mesa_ipv6_t   dhcp6_linkscope_relay_agents_and_servers =
    DHCP6_LINKSCOPE_RELAY_AGENTS_AND_SERVERS_INIT;

const mesa_ipv6_t   dhcp6_sitescope_all_dhcp_servers =
    DHCP6_SITESCOPE_ALL_DHCP_SERVERS_INIT;

const mesa_ipv6_t   dhcp6_unspecified_address =
    DHCP6_UNSPECIFIED_ADDRESS_INIT;

enum ServiceRole {
    ROLE_CLIENT                         = 1,
    ROLE_SERVER                         = 2,
    ROLE_RAGENT                         = 3,
    ROLE_NONE
};

enum IaType {
    IA_TYPE_NA                          = 1,
    IA_TYPE_TA                          = 2,
    IA_TYPE_INVALID
};

enum MessageType {
    DHCP6RESERVED                       = 0,
    DHCP6SOLICIT                        = 1,
    DHCP6ADVERTISE                      = 2,
    DHCP6REQUEST                        = 3,
    DHCP6CONFIRM                        = 4,
    DHCP6RENEW                          = 5,
    DHCP6REBIND                         = 6,
    DHCP6REPLY                          = 7,
    DHCP6RELEASE                        = 8,
    DHCP6DECLINE                        = 9,
    DHCP6RECONFIGURE                    = 10,
    DHCP6INFORMATION_REQUEST            = 11,
    DHCP6RELAY_FORW                     = 12,
    DHCP6RELAY_REPL                     = 13,
    DHCP6LEASEQUERY                     = 14,
    DHCP6LEASEQUERY_REPLY               = 15,
    DHCP6LEASEQUERY_DONE                = 16,
    DHCP6LEASEQUERY_DATA                = 17,
    DHCP6RECONFIGURE_REQUEST            = 18,
    DHCP6RECONFIGURE_REPLY              = 19,

    DHCP6UNASSIGNED
};

#define DHCP6_MSG_INIT                  DHCP6RESERVED
#define DHCP6_MSG_VALID(x)              ((x) < DHCP6UNASSIGNED)

enum OptionCode {
    OPT_RESERVED                        = 0,
    OPT_CLIENTID                        = 1,
    OPT_SERVERID                        = 2,
    OPT_IA_NA                           = 3,
    OPT_IA_TA                           = 4,
    OPT_IAADDR                          = 5,
    OPT_ORO                             = 6,
    OPT_PREFERENCE                      = 7,
    OPT_ELAPSED_TIME                    = 8,
    OPT_RELAY_MSG                       = 9,
    OPT_UNASSIGNED_10                   = 10,
    OPT_AUTH                            = 11,
    OPT_UNICAST                         = 12,
    OPT_STATUS_CODE                     = 13,
    OPT_RAPID_COMMIT                    = 14,
    OPT_USER_CLASS                      = 15,
    OPT_VENDOR_CLASS                    = 16,
    OPT_VENDOR_OPTS                     = 17,
    OPT_INTERFACE_ID                    = 18,
    OPT_RECONF_MSG                      = 19,
    OPT_RECONF_ACCEPT                   = 20,
    OPT_SIP_SERVER_D                    = 21,
    OPT_SIP_SERVER_A                    = 22,
    OPT_DNS_SERVERS                     = 23,
    OPT_DOMAIN_LIST                     = 24,
    OPT_IA_PD                           = 25,
    OPT_IAPREFIX                        = 26,
    OPT_NIS_SERVERS                     = 27,
    OPT_NISP_SERVERS                    = 28,
    OPT_NIS_DOMAIN_NAME                 = 29,
    OPT_NISP_DOMAIN_NAME                = 30,
    OPT_SNTP_SERVERS                    = 31,
    OPT_INFORMATION_REFRESH_TIME        = 32,
    OPT_BCMCS_SERVER_D                  = 33,
    OPT_BCMCS_SERVER_A                  = 34,
    OPT_UNASSIGNED_35                   = 35,
    OPT_GEOCONF_CIVIC                   = 36,

    OPT_UNKNOWN
};

#define DHCP6_OPT_CODE_INIT             OPT_RESERVED
#define DHCP6_OPT_CODE_VALID(x)         ((x) < OPT_UNKNOWN)

enum StatusCode {
    STATUS_SUCCESS                      = 0,
    STATUS_UNSPEC_FAIL                  = 1,
    STATUS_NO_ADDRS_AVAIL               = 2,
    STATUS_NO_BINDING                   = 3,
    STATUS_NOT_ON_LINK                  = 4,
    STATUS_USE_MULTICAST                = 5,
    STATUS_NO_PREFIX_AVAIL              = 6,
    STATUS_UNKNOWN_QUERY_TYPE           = 7,
    STATUS_MALFORMED_QUERY              = 8,
    STATUS_NOT_CONFIGURED               = 9,
    STATUS_NOT_ALLOWED                  = 10,
    STATUS_QUERY_TERMINATED             = 11,

    STATUS_UNASSIGNED
};

#define DHCP6_STATUS_CODE_INIT          STATUS_SUCCESS
#define DHCP6_STATUS_CODE_VALID(x)      ((x) < STATUS_UNASSIGNED)

namespace client
{
enum ServiceType {
    TYPE_NONE                           = 0,
    TYPE_SLAAC                          = 1,
    TYPE_DHCP                           = 2,
    TYPE_SLAAC_AND_DHCP                 = 4,
    TYPE_END
};
} /* client */
} /* dhcp6 */
} /* vtss */

#endif /* _VTSS_DHCP6_TYPE_HXX_ */
