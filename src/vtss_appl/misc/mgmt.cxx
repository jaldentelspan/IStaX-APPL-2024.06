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

#include "main.h"

#ifdef VTSS_SW_OPTION_MAC
#include "mac_api.h"
#endif

#ifdef VTSS_SW_OPTION_ACL
#include "acl_api.h"
#endif

#ifdef VTSS_SW_OPTION_IP
#include "ip_api.h"
#endif

#include "port_iter.hxx"

#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"
#endif

#include "misc_api.h"
#include "mgmt_api.h"
#include "misc.h"
#include <arpa/inet.h>
#include "vtss/appl/types.hxx"
#include "vlan_api.h"
#include "port_api.h" /* For port_count_max() */

#if defined(VTSS_SW_OPTION_QOS) || defined(VTSS_SW_OPTION_ACL)
/****************************************************************************/
/*  QoS/ACL                                                                 */
/****************************************************************************/
/* Convert string to lower case */
static char *mgmt_to_lower(char *buf)
{
    char *p;

    for (p = buf; *p != '\0'; p++)
        *p = tolower(*p);

    return buf;
}

/* ACL IP text (Also used by QoS) */
char *mgmt_acl_ipv4_txt(mesa_ace_ip_t *ip, char *buf, BOOL lower)
{
    ulong i, n = 0;

    if (ip->mask == 0)
        strcpy(buf, "Any");
    else {
        for (i = 0; i < 32; i++)
            if (ip->mask & (1<<i))
                n++;
        misc_ipv4_txt(ip->value, buf);
        sprintf(&buf[strlen(buf)], "/" VPRIlu, n);
    }
    return (lower ? mgmt_to_lower(buf) : buf);
}

mesa_rc mgmt_txt2rate_v2(char *buf, ulong *rate, ulong rate_unit, ulong max_pkt_rate, ulong max_bit_rate, ulong bit_rate_granularity)
{
    mesa_rc rc;
    ulong   val;

    if ((rc = mgmt_txt2ulong(buf, &val, 0, max_pkt_rate)) == VTSS_RC_OK) {
        /* Legal integer, check if rate is one of the legal values */
        if ((rate_unit == 0 && val <= max_pkt_rate) ||
            (rate_unit == 1 && val <= max_pkt_rate && (val % bit_rate_granularity == 0))) {
            *rate = val;
        } else {
            rc = VTSS_UNSPECIFIED_ERROR;
        }
    }
    return rc;
}

/* Convert text to packet rate
 * The string must be one of the following values:
 * 1, 2, 4, ..., 512, 1k, 2k, 4k, ..., 1024k, ...
 * The upper limit is (2^n)k, where 10 <= n <= 16.
 * If n is 10 then the upper limit is (2^10)k = 1024k.
 * If n is 16 then the upper limit is (2^16)k = 65536k.
 */
mesa_rc mgmt_txt2rate(char *buf, ulong *rate, ulong n)
{
    mesa_rc rc;
    int     len;
    ulong   val, i, j, unit = 1;

    if ((n < 1) || (n > 16)) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    len = strlen(buf);
    if (buf[len - 1] == 'k') {
        unit = 1000;
        buf[len - 1] = '\0';
    }

    if ((rc = mgmt_txt2ulong(buf, &val, 1, n < 10 ? 512 : 1<<n)) == VTSS_RC_OK) {
        /* Legal integer, check if rate is one of the legal values */
        if (unit == 1 && val >= 1000 && (val % 1000) == 0) {
            unit = 1000;
            val /= 1000;
        }

        rc = VTSS_UNSPECIFIED_ERROR;
        j = n < 10 ? 9 : n;
        for (i = 0; i <= j; i++) {
            if ((1<<i) == val && ((i < 10 && unit == 1) || (i <= n && unit == 1000))) {
                rc = VTSS_RC_OK;
                *rate = (val * unit);
                break;
            }
        }
    }
    return rc;
}

/* Convert priority to text */
const char *mgmt_prio2txt(mesa_prio_t prio, BOOL lower)
{
    static const char * const txt[] = {"0","1","2","3","4","5","6","7","?"}; // This should be thread safe
    mesa_prio_t uprio = iprio2uprio(prio);
    return txt[uprio > VTSS_PRIO_END ? VTSS_PRIO_END : uprio];
}

/* Convert text to priority */
mesa_rc mgmt_txt2prio(char *buf, mesa_prio_t *cls)
{
    mesa_rc     rc;
    ulong       val;

    if ((rc = mgmt_txt2ulong(buf, &val, iprio2uprio(0), iprio2uprio(VTSS_PRIOS - 1))) == VTSS_RC_OK)
        val = uprio2iprio(val);
    if (rc == VTSS_RC_OK)
        *cls = val;
    return rc;
}
#endif /* defined(VTSS_SW_OPTION_QOS) || defined(VTSS_SW_OPTION_ACL) */




#ifdef VTSS_SW_OPTION_ACL
/****************************************************************************/
/*  ACL                                                                     */
/****************************************************************************/

/* Get ACE ID for SIP/SMAC binding */
mesa_ace_id_t mgmt_acl_ace_id_bind_get(vtss_isid_t isid, mesa_port_no_t port_no, BOOL sip)
{
    /* For each switch (ISID), a block with two ACEs per port is reserved */
    return (ACL_MGMT_ACE_ID_END - (VTSS_ISID_END - isid) * fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * 2 +
            (port_no - VTSS_PORT_NO_START) * 2 + (sip ? 1 : 2));
}

/* ACL frame type text */
char *mgmt_acl_type_txt(mesa_ace_type_t type, char *buf, BOOL lower)
{
    switch (type) {
    case MESA_ACE_TYPE_ANY:
        strcpy(buf, "Any");
        break;
    case MESA_ACE_TYPE_LLC:
        strcpy(buf, "LLC");
        break;
    case MESA_ACE_TYPE_SNAP:
        strcpy(buf, "SNAP");
        break;
    case MESA_ACE_TYPE_ETYPE:
        strcpy(buf, "EType");
        break;
    case MESA_ACE_TYPE_ARP:
        strcpy(buf, "ARP");
        break;
    case MESA_ACE_TYPE_IPV4:
        strcpy(buf, "IPv4");
        break;
    case MESA_ACE_TYPE_IPV6:
        strcpy(buf, "IPv6");
        break;
    default:
        strcpy(buf, "?");
        break;
    }
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL uchar text */
char *mgmt_acl_uchar_txt(mesa_ace_u8_t *data, char *buf, BOOL lower)
{
    if (data->mask)
        sprintf(buf, "%d", data->value);
    else
        strcpy(buf, "Any");
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL uchar2 text */
char *mgmt_acl_uchar2_txt(mesa_ace_u16_t *data, char *buf, BOOL lower)
{
    if (data->mask[0] || data->mask[1])
        sprintf(buf, "0x%02x%02x", data->value[0], data->value[1]);
    else
        strcpy(buf, "Any");
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL uchar6 text */
char *mgmt_acl_uchar6_txt(mesa_ace_u48_t *data, char *buf, BOOL lower)
{
    int i, any = 1;

    for (i = 0; i < 6; i++)
        if (data->mask[i])
            any = 0;

    if (any)
        strcpy(buf, "Any");
    else
        misc_mac_txt(data->value, buf);
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL ulong text */
char *mgmt_acl_ulong_txt(ulong value, ulong mask, char *buf, BOOL lower)
{
    if (mask == 0)
        strcpy(buf, "Any");
    else
        sprintf(buf, VPRIlu, value);
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL IPv6 text */
char *mgmt_acl_ipv6_txt(mesa_ace_u128_t *ip, char *buf, BOOL lower)
{
    int i, any = 1;
    mesa_ipv6_t temp_v6addr;

    for (i = 0; i < 16; i++)
        if (ip->mask[i])
            any = 0;

    if (any) {
        strcpy(buf, "Any");
    } else {
        memcpy(temp_v6addr.addr, ip->value, 16);
        (void) misc_ipv6_txt(&temp_v6addr, buf);
    }

    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL DMAC flags text */
char *mgmt_acl_dmac_txt(acl_entry_conf_t *ace, char *buf, BOOL lower)
{
    strcpy(buf,
           VTSS_BF_GET(ace->flags.mask, ACE_FLAG_DMAC_BC) == 0 ? "Any" :
           VTSS_BF_GET(ace->flags.value, ACE_FLAG_DMAC_BC) ? "Broadcast" :
           VTSS_BF_GET(ace->flags.value, ACE_FLAG_DMAC_MC) ? "Multicast" : "Unicast");
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL ARP opcode flags text */
char *mgmt_acl_opcode_txt(acl_entry_conf_t *ace, char *buf, BOOL lower)
{
    strcpy(buf,
           VTSS_BF_GET(ace->flags.mask, ACE_FLAG_ARP_UNKNOWN) == 0 ? "Any" :
           VTSS_BF_GET(ace->flags.value, ACE_FLAG_ARP_UNKNOWN) ? "Other" :
           VTSS_BF_GET(ace->flags.value, ACE_FLAG_ARP_ARP) ? "ARP" : "RARP");
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL flag text */
const char *mgmt_acl_flag_txt(acl_entry_conf_t *ace, acl_flag_t flag, BOOL lower)
{
    if (ace->type == MESA_ACE_TYPE_IPV6) {
        mesa_ace_bit_t bit_value = MESA_ACE_BIT_ANY;

        switch (flag) {
            case ACE_FLAG_IP_TTL:
                bit_value = ace->frame.ipv6.ttl;
                break;
            case ACE_FLAG_TCP_FIN:
                bit_value = ace->frame.ipv6.tcp_fin;
                break;
            case ACE_FLAG_TCP_SYN:
                bit_value = ace->frame.ipv6.tcp_syn;
                break;
            case ACE_FLAG_TCP_RST:
                bit_value = ace->frame.ipv6.tcp_rst;
                break;
            case ACE_FLAG_TCP_PSH:
                bit_value = ace->frame.ipv6.tcp_psh;
                break;
            case ACE_FLAG_TCP_ACK:
                bit_value = ace->frame.ipv6.tcp_ack;
                break;
            case ACE_FLAG_TCP_URG:
                bit_value = ace->frame.ipv6.tcp_urg;
                break;
            default:
                break;
        }
        return (bit_value == MESA_ACE_BIT_ANY ? (lower ? "any" : "Any") : (bit_value == MESA_ACE_BIT_0 ? "0" : "1"));
    }
    return (VTSS_BF_GET(ace->flags.mask, flag) ?
            VTSS_BF_GET(ace->flags.value, flag) ? "1" : "0" : lower ? "any" : "Any");
}

/* UDP/TCP port text */
char *mgmt_acl_port_txt(mesa_ace_udp_tcp_t *port, char *buf, BOOL lower)
{
    if (port->low == 0 && port->high == 0xffff)
        strcpy(buf, "Any");
    else if (port->low == port->high)
        sprintf(buf, "%d", port->low);
    else
        sprintf(buf, "%d-%d", port->low, port->high);
    return (lower ? mgmt_to_lower(buf) : buf);
}

/* ACL IP protocol number */
uchar mgmt_acl_ip_proto(acl_entry_conf_t *ace)
{
    /* If IPv4 fragment or options, zero is returned */
    if (ace->type == MESA_ACE_TYPE_IPV4 &&
            ace->frame.ipv4.proto.mask &&
            (VTSS_BF_GET(ace->flags.mask, ACE_FLAG_IP_FRAGMENT) == 0 ||
             VTSS_BF_GET(ace->flags.value, ACE_FLAG_IP_FRAGMENT) == 0) &&
            (VTSS_BF_GET(ace->flags.mask, ACE_FLAG_IP_OPTIONS) == 0 ||
             VTSS_BF_GET(ace->flags.value, ACE_FLAG_IP_OPTIONS) == 0)) {
        return ace->frame.ipv4.proto.value;
    } else if (ace->type == MESA_ACE_TYPE_IPV6 && ace->frame.ipv6.proto.mask) {
        return ace->frame.ipv6.proto.value;
    } else {
        return 0;
    }
}
#endif /* VTSS_SW_OPTION_ACL */



#ifdef VTSS_SW_OPTION_PORT



/****************************************************************************/
/*  list                                                                    */
/****************************************************************************/

/* Convert list to text - Named this way because mgmt_list2txt was already made.
 * The resulting buf will contain numbers in range [min; max].
 */
char *mgmt_non_portlist2txt(BOOL *list, int min, int max, char *buf)
{
    int  i, first = 1, count = 0;
    BOOL member;
    char *p;

    p = buf;
    *p = '\0';
    for (i = min; i <= max; i++) {
        member = list[i];
        if ((member && (count == 0 || i == max)) || (!member && count > 1)) {
            p += sprintf(p, "%s%d",
                         first ? "" : count > (member ? 1 : 2) ? "-" : ",",
                         (member ? i : i-1));
            first = 0;
        }
        if (member)
            count++;
        else
            count=0;
    }
    return buf;
}




/****************************************************************************/
/*  Port                                                                    */
/****************************************************************************/

/* Convert list to text
 * The resulting buf will contain numbers in range [min + 1; max + 1]
 * or [min; max] if #one_based is FALSE.
 * The case where #one_based == TRUE is a legacy case, that actually
 * never should have been implemented, or implemented differently.
 * It should have been using iport2uport(i) and iport2uport(i - 1)
 */
static char *list2txt(BOOL *list, int min, int max, char *buf, BOOL bf, BOOL one_based)
{
    int  i, first = 1, count = 0;
    BOOL member;
    char *p;

    p = buf;
    *p = '\0';
    for (i = min; i <= max; i++) {
        member = bf ? VTSS_BF_GET(list, i) : list[i];
        if ((member && (count == 0 || i == max)) || (!member && count > 1)) {
            p += sprintf(p, "%s%d",
                         first ? "" : count > (member ? 1 : 2) ? "-" : ",",
                         one_based ? (member ? i + 1 : i) : (member ? i : i - 1));
            first = 0;
        }
        if (member)
            count++;
        else
            count=0;
    }
    return buf;
}

/* Convert bitfield list to text
 * The resulting buf will contain numbers in range [min; max]
 */
char *mgmt_bf2txt(BOOL *list, int min, int max, char *buf)
{
    return list2txt(list, min, max, buf, 1, 0);
}

/* Convert list to text
 * For backward compatibility, the resulting
 * buf will contain numbers in range [min + 1; max] + 1].
 */
char *mgmt_list2txt(BOOL *list, int min, int max, char *buf)
{
    return list2txt(list, min, max, buf, 0, 1);
}

/* Convert a ulong to a list of booleans
 */
BOOL *mgmt_ulong2bool_list(ulong number,BOOL *bool_list) {
    u16 i;
    for (i = 0; i < 31 ; i++) {
        *bool_list = (number>> i & 0x1);
        bool_list++;

    }
    return bool_list;
}


/* Convert a list of booleans to ulong
 */
u32 mgmt_bool_list2ulong(BOOL *bool_list) {
    u16 i;
    u32 number = 0;
    for (i = 0; i < 31 ; i++) {
        number += (bool_list[i] << i);
    }
    return number;
}


/* Convert port list to text
 * list is indexed by internal port numbers.
 * and the resulting list is in user ports.
 */
char *mgmt_iport_list2txt(const mesa_port_list_t &port_list, char *buf)
{
    mesa_port_no_t iport;
    BOOL           list[64];

    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
        list[iport] = port_list[iport];
    }
    return mgmt_list2txt(list, VTSS_PORT_NO_START, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1, buf);
}

/* Convert text to list or bit field */
mesa_rc mgmt_txt2list_bf(char *buf, BOOL *list, ulong min, ulong max, BOOL def, BOOL bf)
{
    ulong i, start = 0, n;
    char  *p, *end;
    BOOL  error, range = 0, comma = 0;

    /* Clear list by default */
    if (bf) {
        VTSS_BF_CLR(list, max + 1);
    } else {
        for (i = min; i <= max; i++)
            list[i] = 0;
    }

    p = buf;
    error = (p == NULL);
    while (p != NULL && *p != '\0') {
        /* Read integer */
        n = strtoul(p, &end, 0);
        if (end == p) {
            error = 1;
            break;
        }
        p = end;

        /* Check legal range */
        if (n < min || n > max) {
            error = 1;
            break;
        }

        if (range) {
            /* End of range has been read */
            if (n < start) {
                error = 1;
                break;
            }
            for (i = start ; i <= n; i++) {
                if (bf) {
                    VTSS_BF_SET(list, i, 1);
                } else
                    list[i] = 1;
            }
            range = 0;
        } else if (*p == '-') {
            /* Start of range has been read */
            start = n;
            range = 1;
            p++;
        } else {
            /* Single value has been read */
            if (bf) {
                VTSS_BF_SET(list, n, 1);
            } else
                list[n] = 1;
        }
        comma = 0;
        if (!range && *p == ',') {
            comma = 1;
            p++;
        }
    }

    /* Check for trailing comma/dash */
    if (comma || range)
        error = 1;

    /* Restore defaults if error */
    if (error) {
        if (bf) {
            memset(list, def ? 0xff : 0x00, VTSS_BF_SIZE(max + 1));
        } else {
            for (i = min; i <= max; i++)
                list[i] = def;
        }
    }
    return (error ? VTSS_UNSPECIFIED_ERROR : VTSS_RC_OK);
}

/* Convert text to list */
mesa_rc mgmt_txt2list(char *buf, BOOL *list, ulong min, ulong max, BOOL def)
{
    return mgmt_txt2list_bf(buf, list, min, max, def, 0);
}

/* Convert text to bit field */
mesa_rc mgmt_txt2bf(char *buf, BOOL *list, ulong min, ulong max, BOOL def)
{
    return mgmt_txt2list_bf(buf, list, min, max, def, 1);
}

/* Convert user-port text to uport list */
mesa_rc mgmt_uport_txt2list(char *buf, BOOL *list)
{
    return mgmt_txt2list(buf, list, 1, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), 0);
}

/* Convert text to range */
mesa_rc mgmt_txt2range(char *buf, ulong *req_min, ulong *req_max, ulong min, ulong max)
{
    ulong x, y = 0;
    char  *end;

    x = strtoul(buf, &end, 0);
    if (*end == '\0')
        y = x;
    else if (*end == '-')
        y = strtoul(end + 1, &end, 0);

    if (*end != '\0' || x < min || y > max || x > y ) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    *req_min = x;
    *req_max = y;
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_PORT */


/****************************************************************************/
/*  IP                                                                      */
/****************************************************************************/

/* Validate and convert IPv4 address string a.b.c.d[/n]
 * If is_mask is true, the ip parameter is expected to be a subnet mask.
 * If mask != NULL an optional number of mask bits is allowed (CIDR notation).
 * If the optional number of mask bits is present, mask is calculated from here,
 * otherwise mask is set to all ones.
 * If mask == NULL, only a legal unicast address or 0.0.0.0 is accepted.
 */
mesa_rc mgmt_txt2ipv4_ext(const char *buf, mesa_ipv4_t *ip, mesa_ipv4_t *mask, BOOL is_mask, BOOL allow_loopback, BOOL allow_ipmc, BOOL allow_ip_zero)
{
    uint  a, b, c, d, n = 0;
    char  maskbits[8]; // Reserve space for null terminator
    char  garbage[2];  // Reserve space for null terminator
    char  newbuf[16]; //
    int   items, new_items, i;
    BOOL  zero;
    ulong data;
    struct in_addr host;

    // Check basic ipv4 format
    items = sscanf(buf, "%u.%u.%u.%u%7s", &a, &b, &c, &d, maskbits);
    if (items < 4) {
        /* ERROR: Invalid IP address format */
        return VTSS_UNSPECIFIED_ERROR;
    }

    // Create a new buffer with the IP address only
    sprintf(newbuf,"%u.%u.%u.%u", a, b, c, d);

    // Translate the Hex-based/Octal-based IP address to the Decimal-based IP address
    if (inet_aton(newbuf, &host)) {
        misc_ipv4_txt(ntohl(host.s_addr), newbuf);
    } else {
        //printf("VTSS_UNSPECIFIED_ERROR\n");
        return VTSS_UNSPECIFIED_ERROR;
    }

    new_items = sscanf(newbuf, "%u.%u.%u.%u", &a, &b, &c, &d);
    if ((new_items != 4) || (a > 255) || (b > 255) || (c > 255) || (d > 255)) {
        /* ERROR: Invalid IP address format */
        return VTSS_UNSPECIFIED_ERROR;
    }

    if (items == 5) {
        if (mask == NULL) {
            /* ERROR: Nothing is allowed after the IP address unless mask != NULL */
            return VTSS_UNSPECIFIED_ERROR;
        }
        if (maskbits[0] == '/' && ((sscanf(maskbits, "/%u%1s", &n, garbage) != 1) || (n < 1) || (n > 32))) {
            /* ERROR: Invalid mask bits */
            return VTSS_UNSPECIFIED_ERROR;
        }
    }

    data = ((a<<24) + (b<<16) + (c<<8) + d);

    if (is_mask) {
        /* Check that mask is contiguous */
        for (i = 0, zero = 0; i < 32; i++) {
            if (!(data & (1<<(31-i)))) {
                /* Cleared bit was found */
                zero = 1;
            } else if (zero) {
                /* ERROR: Set bit was found and cleared bit was previously found */
                return VTSS_UNSPECIFIED_ERROR;
            }
        }
    } else if (mask == NULL) {
        /* Check that if we need to allow 0.0.0.0 (or) a legal unicast address (or) a loopback address (or) a multicast address*/
        if (((data == 0 && !allow_ip_zero) || ((a == 0) && (data != 0)) || (a == 127 && !allow_loopback) || (a > 223 && !allow_ipmc))) {
            /* ERROR: Not a legal unicast address or 0.0.0.0 */
            return VTSS_UNSPECIFIED_ERROR;
        }
    }

    if (mask) {
        if (items == 4) {
            *mask = 0xffffffff;
        }
        else {
            *mask = 0;
            for (i = 0; i < n; i++) {
                *mask |= (1<<(31-i));
            }
        }
    }

    *ip = data;
    return VTSS_RC_OK;
}

/* Validate and convert IPv4 address string a.b.c.d[/n]
 * If is_mask is true, the ip parameter is expected to be a subnet mask.
 * If mask != NULL an optional number of mask bits is allowed (CIDR notation).
 * If the optional number of mask bits is present, mask is calculated from here,
 * otherwise mask is set to all ones.
 * If mask == NULL, only a legal unicast address or 0.0.0.0 is accepted.
 */
mesa_rc mgmt_txt2ipv4(const char *buf, mesa_ipv4_t *ip, mesa_ipv4_t *mask, BOOL is_mask)
{
    if (mask == NULL) {
        return mgmt_txt2ipv4_ext(buf, ip, mask, is_mask, FALSE, FALSE, TRUE);
    } else {
        return mgmt_txt2ipv4_ext(buf, ip, mask, is_mask, FALSE, FALSE, FALSE);
    }
}

#if defined(VTSS_SW_OPTION_IP)
/* Validate and convert IPv4 multicast address string a.b.c.d
 * Valid IPv4 multicast address range is 224.0.0.0 to 239.255.255.255
 */
mesa_rc mgmt_txt2ipmcv4(const char *buf, mesa_ipv4_t *ip)
{
    uint  a, b, c, d;
    char  garbage[2]; // Reserve space for null terminator
    int   items;

    items = sscanf(buf, "%u.%u.%u.%u%1s", &a, &b, &c, &d, garbage);
    // Check that there is excactly 4 items read.
    // If the buf contains trailing garbage, items would be 5.
    if ((items != 4) || (a < 224) || (a > 239) || (b > 255) || (c > 255) || (d > 255)) {
        return VTSS_UNSPECIFIED_ERROR;
    }
    *ip = ((a<<24) + (b<<16) + (c<<8) + d);
    return VTSS_RC_OK;
}

#define TXT2IPV6_INVALID_BIN_VAL    (-1)
#define TXT2IPV6_VAL_BIN_MASK       0xF
#define TXT2IPV6_VAL_MAX_SIGN       0xFFFF
#define TXT2IPV6_MIN_COL_CNT        2
#define TXT2IPV6_MAX_COL_CNT        7
#define TXT2IPV6_MIN_DOT_CNT        3
#define TXT2IPV6_MAX_DOT_CNT        3
#define TXT2IPV6_MAX_ELEMENT_CNT    8
#define TXT2IPV6_MAX_SYMBOL_CNT     9
#define TXT2IPV6_MAX_DOT_VAL        0x9
#if IPV6_ADDR_STR_LEN_LONG
#define TXT2IPV6_MAX_COL_DIGIT_CNT  (IPV6_ADDR_STR_MAX_LEN - IPV6_ADDR_STR_MIN_LEN)
#else
#define TXT2IPV6_MAX_COL_DIGIT_CNT  4
#endif /* IPV6_ADDR_STR_LEN_LONG  */
#define TXT2IPV6_MAX_DOT_DIGIT_CNT  3
#define TXT2IPV6_HEX_VAL_SHIFT_UNIT 4
#define TXT2IPV6_DEC_VAL_SHIFT_UNIT 10
#define TXT2IPV6_V6_ADDR_SHIFT_UNIT 8
#define TXT2IPV6_V6_ADDR_SHIFT_MASK 0xFF
#define TXT2IPV6_V6_ADDR_ZERO_SIGN  0x0
#define TXT2IPV6_IPV4COMPAT_SIGN    0x0
#define TXT2IPV6_IPV4MAPPED_SIGN    0xFFFF
#define TXT2IPV6_V4TOV6_ELECNT_BASE 2
#define TXT2IPV6_V4TOV6_COMPAT_BASE 12
#define TXT2IPV6_V4TOV6_MAPPED_BASE 10
#define TXT2IPV6_V4TOV6_MAPPED_CHK1 11
#define TXT2IPV6_V4TOV6_MAPPED_SIGN 0xFF
#define TXT2IPV6_V4TOV6_MAX_OFFSET  3
#define TXT2IPV6_V4TOV6_MAX_VALUE   0xFF
#define TXT2IPV6_POWER_BASE_VAL     1
#define TXT2IPV6_ADDR_IDX_BASE      0x0
#define TXT2IPV6_ADDR_IDX_SIZE      sizeof(mesa_ipv6_t)
#define TXT2IPV6_ADDR_IDX_END       (TXT2IPV6_ADDR_IDX_SIZE - 1)
#define TXT2IPV6_V4_MCAST_LEADING   0xE0
#define TXT2IPV6_V6_MCAST_LEADING   0xFF
#define TXT2IPV6_ADDR_LL_CHK_IDX0   TXT2IPV6_ADDR_IDX_BASE
#define TXT2IPV6_ADDR_LL_CHK_IDX1   1
#define TXT2IPV6_ADDR_LB_CHK_IDX    TXT2IPV6_ADDR_IDX_END
#define TXT2IPV6_ADDR_LL_SIGN1      0xFE
#define TXT2IPV6_ADDR_LL_SIGN2      0x80
#define TXT2IPV6_ADDR_LB_SIGN       0x1

static int hex2bin(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    return TXT2IPV6_INVALID_BIN_VAL;
}

static txt2ip6_addr_type_t _mgmt_txt2ipv6(const char *s, mesa_ipv6_t *addr)
{
    /* Follow RFC-4291 & RFC-5952 */
    u8      i, j, k, idx, len, val_cnt, dot_cnt, col_cnt;
    u16     curr_val, last_val, prev_val;
    u32     max_hex_val;
    int     val;
    BOOL    flag, val_chk, hex_val;
    BOOL    ipv4_compat, ipv4_mapped;

    if (!s || !addr) {
        return TXT2IP6_ADDR_TYPE_INVALID;
    }

    /* except the "...%iface" part of link-local addresses */
    len = strchrnul(s, int('%')) - s;
    if ((len < IPV6_ADDR_STR_MIN_LEN) || (len > IPV6_ADDR_STR_MAX_LEN)) {
        return TXT2IP6_ADDR_TYPE_INVALID;
    }

    /* syntax parsing first */
    val_cnt = dot_cnt = col_cnt = 0;
    curr_val = last_val = prev_val = 0;
    max_hex_val = 0;
    flag = hex_val = val_chk = FALSE;
    ipv4_compat = ipv4_mapped = FALSE;
    for (i = 0; i < len; i++) {
        if ((val = hex2bin(s[i])) != TXT2IPV6_INVALID_BIN_VAL) {
            if (!val_cnt) {
                if (!prev_val) {
                    prev_val = last_val;
                }
                last_val = curr_val;
                curr_val = 0;
                max_hex_val = 0;

                if (dot_cnt) {
                    hex_val = FALSE;
                    val_chk = TRUE;
                } else {
                    hex_val = TRUE;
                    val_chk = FALSE;
                }
            }

            for (j = i + 1; !val_chk && (j < len); j++) {
                if (s[j] == '.') {
                    hex_val = FALSE;
                    break;
                } else if (s[j] == ':') {
                    hex_val = TRUE;
                    break;
                } else {
                    if (hex2bin(s[j]) == TXT2IPV6_INVALID_BIN_VAL) {
                        return TXT2IP6_ADDR_TYPE_INVALID;
                    }
                }
            }
            val_chk = TRUE;

            val_cnt++;

            if (hex_val) {
                if (val_cnt > TXT2IPV6_MAX_COL_DIGIT_CNT) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }

                if (val_cnt - 1) {
                    max_hex_val = max_hex_val << TXT2IPV6_HEX_VAL_SHIFT_UNIT;
                }

                max_hex_val |= (val & TXT2IPV6_VAL_BIN_MASK);
                if (max_hex_val > TXT2IPV6_VAL_MAX_SIGN) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                } else {
                    curr_val = max_hex_val & TXT2IPV6_VAL_MAX_SIGN;
                }
            } else {
                if (val_cnt > TXT2IPV6_MAX_DOT_DIGIT_CNT) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }

                if (val > TXT2IPV6_MAX_DOT_VAL) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }

                curr_val = (curr_val * TXT2IPV6_DEC_VAL_SHIFT_UNIT) + val;
            }

            continue;
        }

        if (s[i] == ':') {
            if (dot_cnt) {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }
            if (!col_cnt && ((i + 1) < len)) {
                if (!val_cnt && (hex2bin(s[i + 1]) != TXT2IPV6_INVALID_BIN_VAL)) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }
            }

            if (((i + 1) < len) && (s[i + 1] == ':')) {
                if (!flag) {
                    flag = TRUE;
                } else {
                    /* :: is only allowed once */
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }
            }

            col_cnt++;

            if (col_cnt > TXT2IPV6_MAX_COL_CNT) {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }

            val_cnt = 0;
            continue;
        }

        if (s[i] == '.') {
            if (!col_cnt || !val_cnt) {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }

            if (!dot_cnt) {
                if (prev_val) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }

                /* Check DOTTED IPv4-Mapped or IPv4-Compatible(Deprecated) */
                if (last_val == TXT2IPV6_IPV4COMPAT_SIGN) {
                    ipv4_compat = TRUE;
                } else if (last_val == TXT2IPV6_IPV4MAPPED_SIGN) {
                    ipv4_mapped = TRUE;
                } else {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }
            }

            if ((i + 1) < len) {
                if (hex2bin(s[i + 1]) == TXT2IPV6_INVALID_BIN_VAL) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }
            } else {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }

            if (curr_val > TXT2IPV6_V4TOV6_MAX_VALUE) {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }

            dot_cnt++;

            if (dot_cnt > TXT2IPV6_MAX_DOT_CNT) {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }

            val_cnt = 0;
            continue;
        }

        return TXT2IP6_ADDR_TYPE_INVALID;
    }
    if (col_cnt < TXT2IPV6_MIN_COL_CNT) {
        return TXT2IP6_ADDR_TYPE_INVALID;
    }
    if (dot_cnt) {
        if (dot_cnt < TXT2IPV6_MIN_DOT_CNT) {
            return TXT2IP6_ADDR_TYPE_INVALID;
        }

        if ((dot_cnt + col_cnt) > TXT2IPV6_MAX_SYMBOL_CNT) {
            return TXT2IP6_ADDR_TYPE_INVALID;
        }

        if (!flag) {
            j = TXT2IPV6_V4TOV6_ELECNT_BASE;    /* count element */
            for ((val_cnt = 0), (i = 0); i < len; i++) {
                if (s[i] == '.') {
                    break;
                }

                if (s[i] != ':') {
                    val_cnt++;
                } else {
                    if (val_cnt) {
                        j++;
                    }

                    val_cnt = 0;
                }
            }

            if (j != TXT2IPV6_MAX_ELEMENT_CNT) {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }
        }
    } else {
        if (!flag && (col_cnt != TXT2IPV6_MAX_COL_CNT)) {
            return TXT2IP6_ADDR_TYPE_INVALID;
        }
    }

    /* assign value (IPv6 Address) second/next */
    if (ipv4_compat || ipv4_mapped) {
        u8  v4_val_set = 0;

        memset(addr, 0x0, sizeof(mesa_ipv6_t));
        j = TXT2IPV6_V4TOV6_MAX_OFFSET;
        for ((val_cnt = 0), (i = len); i > 0; i--) {
            if (s[i - 1] == ':') {
                addr->addr[TXT2IPV6_V4TOV6_COMPAT_BASE] = v4_val_set;

                if (s[i - 2] == ':') {
                    break;
                } else {
                    if (hex2bin(s[i - 2])) {
                        addr->addr[TXT2IPV6_V4TOV6_MAPPED_BASE] = TXT2IPV6_V4TOV6_MAPPED_SIGN;
                        addr->addr[TXT2IPV6_V4TOV6_MAPPED_CHK1] = TXT2IPV6_V4TOV6_MAPPED_SIGN;
                    }
                }

                break;
            }

            if (s[i - 1] == '.') {
                addr->addr[TXT2IPV6_V4TOV6_COMPAT_BASE + j] = v4_val_set;

                if (j == 0) {
                    break;
                }

                j--;
                val_cnt = 0;
                v4_val_set = 0;
            } else {
                val = hex2bin(s[i - 1]);

                k = TXT2IPV6_POWER_BASE_VAL;
                for (idx = 0; idx < val_cnt; idx++) {
                    k = k * TXT2IPV6_DEC_VAL_SHIFT_UNIT;
                }
                v4_val_set = v4_val_set + (val * k);
                val_cnt++;
            }
        }
    } else {
        j = 0;  /* count element */
        for ((val_cnt = 0), (i = 0); i < len; i++) {
            if (s[i] != ':') {
                val_cnt++;

                if ((i + 1) == len) {
                    j++;
                }
            } else {
                if (val_cnt) {
                    j++;
                }

                val_cnt = 0;
            }
        }

        if (!j) {
            memset(addr, 0x0, sizeof(mesa_ipv6_t));
        } else {
            u16 v6_val_set = 0;

            k = TXT2IPV6_MAX_ELEMENT_CNT - j;   /* zero-compress count */
            idx = TXT2IPV6_ADDR_IDX_BASE;
            for ((val_cnt = 0), (i = 0); i < len; i++) {
                if (s[i] == ':') {
                    addr->addr[idx] = v6_val_set >> TXT2IPV6_V6_ADDR_SHIFT_UNIT;
                    idx++;
                    addr->addr[idx] = v6_val_set & TXT2IPV6_V6_ADDR_SHIFT_MASK;
                    idx++;
                    if (idx > TXT2IPV6_ADDR_IDX_END) {
                        break;
                    }

                    if (((i + 1) < len) && (s[i + 1] == ':')) {
                        if (v6_val_set) {
                            j = 0;
                        } else {
                            j = 1;  /* already compress last one */
                        }
                        for (; j < k; j++) {
                            addr->addr[idx] = TXT2IPV6_V6_ADDR_ZERO_SIGN;
                            idx++;
                            if (idx > TXT2IPV6_ADDR_IDX_END) {
                                break;
                            }
                            addr->addr[idx] = TXT2IPV6_V6_ADDR_ZERO_SIGN;
                            idx++;
                            if (idx > TXT2IPV6_ADDR_IDX_END) {
                                break;
                            }
                        }

                        i++;
                    }

                    val_cnt = 0;
                    v6_val_set = 0;
                } else {
                    val_cnt++;

                    val = hex2bin(s[i]);
                    if (val_cnt - 1) {
                        v6_val_set = v6_val_set << TXT2IPV6_HEX_VAL_SHIFT_UNIT;
                    }

                    v6_val_set |= (val & TXT2IPV6_VAL_BIN_MASK);

                    if ((i + 1) == len) {
                        addr->addr[idx] = v6_val_set >> TXT2IPV6_V6_ADDR_SHIFT_UNIT;
                        idx++;
                        addr->addr[idx] = v6_val_set & TXT2IPV6_V6_ADDR_SHIFT_MASK;
                        idx++;
                        if (idx > TXT2IPV6_ADDR_IDX_END) {
                            break;
                        }
                    }
                }
            }
        }
    }

    /*
        Address Type could be determined precisely here/now
        Unspecified Address (ALL-ZERO)
        Unicast
        Multicast
        Link-Local
        Loopback
        IPv4-Mapped or IPv4-Compatible(Deprecated)
        ...
    */
    if (addr->addr[TXT2IPV6_ADDR_IDX_BASE] == TXT2IPV6_V6_MCAST_LEADING) {
        /* Multicast */
        return TXT2IP6_ADDR_TYPE_MCAST;
    } else {
        /* Unicast */
        if ((addr->addr[TXT2IPV6_ADDR_LL_CHK_IDX0] == TXT2IPV6_ADDR_LL_SIGN1) &&
            (addr->addr[TXT2IPV6_ADDR_LL_CHK_IDX1] == TXT2IPV6_ADDR_LL_SIGN2)) {
            /* Link Local */
            return TXT2IP6_ADDR_TYPE_LINK_LOCAL;
        }

        flag = FALSE;
        i = TXT2IPV6_ADDR_IDX_END;
        for (idx = TXT2IPV6_ADDR_IDX_BASE; idx <= i; idx++) {
            if (addr->addr[idx]) {
                flag = TRUE;
                break;
            }
        }

        if (!flag) {
            /* Unspecified */
            return TXT2IP6_ADDR_TYPE_UNSPECIFIED;
        }

        if ((idx == TXT2IPV6_ADDR_LB_CHK_IDX) &&
            (addr->addr[TXT2IPV6_ADDR_LB_CHK_IDX] == TXT2IPV6_ADDR_LB_SIGN)) {
            /* Loopback */
            return TXT2IP6_ADDR_TYPE_LOOPBACK;
        }

        j = addr->addr[TXT2IPV6_V4TOV6_COMPAT_BASE] & TXT2IPV6_V4_MCAST_LEADING;
        if (idx >= TXT2IPV6_V4TOV6_COMPAT_BASE) {
            /* IPv4 Multicast/Broadcast (Leading: 1110) is not allowed */
            if (j == TXT2IPV6_V4_MCAST_LEADING) {
                return TXT2IP6_ADDR_TYPE_INVALID;
            }

            /* IPv4-Compatible(Deprecated) */
            return TXT2IP6_ADDR_TYPE_IPV4_COMPAT;
        }
        if (idx == TXT2IPV6_V4TOV6_MAPPED_BASE) {
            if ((addr->addr[TXT2IPV6_V4TOV6_MAPPED_BASE] == TXT2IPV6_V4TOV6_MAPPED_SIGN) &&
                (addr->addr[TXT2IPV6_V4TOV6_MAPPED_CHK1] == TXT2IPV6_V4TOV6_MAPPED_SIGN)) {
                /* IPv4 Multicast/Broadcast (Leading: 1110) is not allowed */
                if (j == TXT2IPV6_V4_MCAST_LEADING) {
                    return TXT2IP6_ADDR_TYPE_INVALID;
                }

                /* IPv4-Mapped */
                return TXT2IP6_ADDR_TYPE_IPV4_MAPPED;
            }
        }

        /* General Unicast */
        return TXT2IP6_ADDR_TYPE_UCAST;
    }

    return TXT2IP6_ADDR_TYPE_GEN;
}

mesa_rc mgmt_txt2ipv6(const char *s, mesa_ipv6_t *addr)
{
    txt2ip6_addr_type_t retType = _mgmt_txt2ipv6(s, addr);

    if (retType == TXT2IP6_ADDR_TYPE_INVALID) {
        return VTSS_UNSPECIFIED_ERROR;
    } else {
        if (retType == TXT2IP6_ADDR_TYPE_LOOPBACK) {
            return VTSS_UNSPECIFIED_ERROR;
        } else {
            return VTSS_RC_OK;
        }
    }
}

txt2ip6_addr_type_t mgmt_txt2ipv6_type(const char *s, mesa_ipv6_t *addr)
{
    return _mgmt_txt2ipv6(s, addr);
}

#endif /* defined(VTSS_SW_OPTION_IP) */

#ifdef VTSS_SW_OPTION_AGGR
/****************************************************************************/
/*  Aggregation                                                             */
/****************************************************************************/

/* Convert aggregation ID to aggregation number */
aggr_mgmt_group_no_t mgmt_aggr_id2no(aggr_mgmt_group_no_t aggr_id)
{
    return (aggr_id + AGGR_MGMT_GROUP_NO_START - MGMT_AGGR_ID_START);
}

/* Convert aggregation number to aggregation ID */
aggr_mgmt_group_no_t mgmt_aggr_no2id(aggr_mgmt_group_no_t aggr_no)
{
    return (aggr_no + MGMT_AGGR_ID_START - AGGR_MGMT_GROUP_NO_START);
}
#endif /* VTSS_SW_OPTION_AGGR */




#ifdef VTSS_SW_OPTION_SNMP
/****************************************************************************/
/*  SNMP                                                                    */
/****************************************************************************/

/* Convert text (e.g. .1.2.*.3) to OID */
mesa_rc mgmt_txt2oid(char *buf, int len,
                     ulong *oid, uchar *oid_mask, ulong *oid_len)
{
    ulong count = 0;
    int   i, j = 0, c = 0, prev;
    uchar mask = 0;

    for (i = 0; i < len; i++) {
        prev = c;
        c = buf[i];
        if (c == '.') {
            if (prev == '.' || i == (len - 1) || count == 128)
                return VTSS_UNSPECIFIED_ERROR;
            j = count;
            mask = (1<<(7 - (count % 8)));
            count++;
        } else if (c == '*') {
            if (prev != '.')
                return VTSS_UNSPECIFIED_ERROR;
            oid_mask[j/8] &= ~mask;
        } else if (isdigit(c)) {
            if (prev == 0 || prev == '*')
                return VTSS_UNSPECIFIED_ERROR;
            if (prev == '.')
                oid[j] = 0;
            oid[j] = (oid[j]*10 + c - '0');
            oid_mask[j/8] |= mask;
        } else
            return VTSS_UNSPECIFIED_ERROR;
    }

    *oid_len = count;

    return (count ? VTSS_RC_OK : VTSS_UNSPECIFIED_ERROR);
}
#endif /* VTSS_SW_OPTION_SNMP */



/****************************************************************************/
/*  General                                                                 */
/****************************************************************************/

/* Convert text to ulong */
mesa_rc mgmt_txt2ulong(char *buf, ulong *val, ulong min, ulong max)
{
    ulong n;
    ulonglong temp;
    char  *end;

    temp = strtoull(buf, &end, 0);
    if (*end != '\0' || temp > 0xFFFFFFFF)
        return VTSS_UNSPECIFIED_ERROR;

    n = (ulong)temp;
    if (n < min || n > max)
        return VTSS_UNSPECIFIED_ERROR;

    *val = n;
    return VTSS_RC_OK;
}

/* Convert text to signed long (decimal or hex) */
mesa_rc mgmt_txt2long(char *buf, long *val, long min, long max)
{
  long n;
  char  *end;

  // Check is it is a decimal number
  n = strtol(buf, &end, 10);
  if (*end != '\0' || n < min || n > max) {
      
      // It wasn't a decimal number, now check that it is a hex number
      n = strtol(buf, &end, 16);
      if (*end != '\0' || n < min || n > max) {
          // Not a hex number
          return VTSS_UNSPECIFIED_ERROR;
      }
  }
  *val = n;

  return VTSS_RC_OK;
}

//
// We don't want to enable floating point operations, so this is a special
// floating point parse, that take a "floating point string" it t a long.
// E.g. 10.4 (with one digit) is converted to 104.
// E.g. 4.567 (with 3 digit) is converted to 4567.
// E.g. 4.5   (with 3 digit) is converted to 4500.
//
// In : string_value - The value as string
//      min          - The lowest allowed number.
//      max          - The highest allowed number.
//      digi_cnt     - The number of digits.
//
// Out : value       - The value as long
//
// Return : mesa_rc - VTSS_RC_OK if conversion was completed without problems.

mesa_rc mgmt_str_float2long(char *string_value, long *value, long min, ulong max, uchar digi_cnt)
{
    char *dot_location;
    char i;
    char string_value_cpy[200];
    strcpy(string_value_cpy,string_value);

    dot_location = strstr(string_value_cpy,".");
    if (dot_location == NULL) {
        // The string doesn't contain a ".", so we simply adds a 0 to it. E.g 10 is converted to 100
        for (i = 0; i < digi_cnt; i++) {
            strcat(string_value_cpy,"0"); // Add 0 - same as mulitply with 10
        }
    } else {
        // The string contains a ".", so we removes the dot 
        // do not use strcpy on overlapping strings
        memmove(dot_location,dot_location + 1,strlen(dot_location + 1)+1); // Remove the dot

        // Check that the string only contained the allowed number of digits
        if (strlen(dot_location) > digi_cnt) {
            return VTSS_UNSPECIFIED_ERROR;
        } else {
            char digits_to_add = digi_cnt - strlen(dot_location);
            for (i = 0; i < digits_to_add ; i++) {
                strcat(string_value_cpy,"0"); // Add 0 - same as mulitply with 10
            }
        }
    }
    return mgmt_txt2long(string_value_cpy, value, min, max);
}

// We don't want to enable floating point operations, so this is a special
// floating point parse. The opposite of mgmt_str_float2long
//
// In : value        - The value as long
//      digi_cnt     - The number of digits.
//
// Out :string_value - The value as string
//

void mgmt_long2str_float(char *string_value, long value, char digi_cnt)
{
    char int_part[100];
    char fragment[100];
    char long_value_as_string[200];
    char long_value_as_string_len;
    char i;
    int dot_pos;

    sprintf(long_value_as_string,"%ld",value);
    long_value_as_string_len = strlen(long_value_as_string);
    dot_pos = long_value_as_string_len - digi_cnt;

    if (dot_pos <= 0) {
        strcpy(int_part,"0");
        strcpy(fragment,"");
        for (i = 0;i < digi_cnt - long_value_as_string_len; i ++) {
            strcat(fragment,"0");
        }
        strcat(fragment,long_value_as_string);
    } else  {
        strncpy(int_part,long_value_as_string,dot_pos);
        int_part[dot_pos]='\0';
        strcpy(fragment,&long_value_as_string[dot_pos]);
    }

    strcpy(string_value,"");
    strcat(string_value,int_part);
    strcat(string_value,".");
    strcat(string_value,fragment);
}

const char *mgmt_enum_descriptor2txt(const vtss_enum_descriptor_t *descriptor, int value)
{
    VTSS_ASSERT(NULL != descriptor);
    while (true) {
        if (!descriptor->valueName) {
            return "?";
        }

        if (descriptor->intValue == value) {
            return descriptor->valueName;
        }

        descriptor++;
    }

}

BOOL mgmt_types_port_list_2_array(const vtss_port_list_stackable_t *const port_list, const vtss_isid_t isid, mesa_port_list_t &enable)
{
    vtss::PortListStackable     &pls = (vtss::PortListStackable &)(*port_list);
    port_iter_t                 pit;

    if (NULL == port_list || (isid < VTSS_ISID_START || isid >= VTSS_ISID_END)) {
        return FALSE;
    }

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        enable[pit.iport] = pls.get(isid, pit.iport);
    }

    return TRUE;
}

BOOL mgmt_types_array_2_port_list(const vtss_isid_t isid, const mesa_port_list_t &enable, vtss_port_list_stackable_t *const port_list)
{
    vtss::PortListStackable     &pls = (vtss::PortListStackable &)(*port_list);
    port_iter_t                 pit;

    if (NULL == port_list || (isid < VTSS_ISID_START || isid >= VTSS_ISID_END)) {
        return FALSE;
    }

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        pls.set(isid, pit.iport);
    }

    return TRUE;
}

BOOL mgmt_types_port_list_bit_value_get(const vtss_port_list_stackable_t *const port_list, const vtss_isid_t isid, const mesa_port_no_t iport)
{
    vtss::PortListStackable     &pls = (vtss::PortListStackable &)(*port_list);

    VTSS_ASSERT(NULL != port_list && (isid >= VTSS_ISID_START && isid < VTSS_ISID_END) && (iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)));

    return pls.get(isid, iport);

}

void mgmt_types_port_list_bit_value_set(const vtss_port_list_stackable_t *const port_list, const vtss_isid_t isid, const mesa_port_no_t iport)
{
    vtss::PortListStackable     &pls = (vtss::PortListStackable &)(*port_list);

    VTSS_ASSERT(NULL != port_list && (isid >= VTSS_ISID_START && isid < VTSS_ISID_END) && (iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)));

    pls.set(isid, iport);

}

void mgmt_types_port_list_bit_value_clear(const vtss_port_list_stackable_t *const port_list, const vtss_isid_t isid, const mesa_port_no_t iport)
{
    vtss::PortListStackable     &pls = (vtss::PortListStackable &)(*port_list);

    VTSS_ASSERT(NULL != port_list && (isid >= VTSS_ISID_START && isid < VTSS_ISID_END) && (iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)));

    pls.clear(isid, iport);

}

BOOL mgmt_types_port_list_is_empty(const vtss_port_list_stackable_t *const port_list)
{
    vtss::PortListStackable     &pls = (vtss::PortListStackable &)(*port_list);
    switch_iter_t               sit;
    port_iter_t                 pit;
    vtss_ifindex_t              ife;
    BOOL                        found = FALSE;

    VTSS_ASSERT(NULL != port_list);
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (TRUE == pls.get(sit.isid, pit.iport)) {
                found = TRUE;
                goto find_enabled_port;
            }
        }
    }

find_enabled_port:
    if (found) {
        if (VTSS_RC_OK != vtss_ifindex_from_port(sit.isid, pit.iport, &ife)) {
            return TRUE;
        }

        return FALSE;
    }
    return TRUE;
}

BOOL mgmt_types_vlan_list_2_array(const vtss_vlan_list_t *const vlan_list, BOOL *enable, size_t size)
{
    vtss::VlanList      &vls = (vtss::VlanList &)(*vlan_list);
    mesa_vid_t          vid;

    if (NULL == vlan_list || NULL == enable || size != VTSS_APPL_VLAN_ID_MAX) {
        return FALSE;
    }

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; ++vid) {
        enable[vid] = vls.get(vid);
    }

    return TRUE;
}

BOOL mgmt_types_array_2_vlan_list(const BOOL *const enable, size_t size, vtss_vlan_list_t *const vlan_list)
{
    vtss::VlanList      &vls = (vtss::VlanList &)(*vlan_list);
    mesa_vid_t          vid;

    if (NULL == vlan_list || NULL == enable || size != VTSS_APPL_VLAN_ID_MAX) {
        return FALSE;
    }

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; ++vid) {
        vls.set(vid);
    }

    return TRUE;
}

BOOL mgmt_types_vlan_list_bit_value_get(const vtss_vlan_list_t *const vlan_list, const mesa_vid_t vid)
{
    vtss::VlanList     &vls = (vtss::VlanList &)(*vlan_list);

    VTSS_ASSERT(NULL != vlan_list && (vid >= VTSS_APPL_VLAN_ID_MIN && vid <= VTSS_APPL_VLAN_ID_MAX));

    return vls.get(vid);

}

void mgmt_types_vlan_list_bit_value_set(const vtss_vlan_list_t *const vlan_list, const mesa_vid_t vid)
{
    vtss::VlanList     &vls = (vtss::VlanList &)(*vlan_list);

    VTSS_ASSERT(NULL != vlan_list && (vid >= VTSS_APPL_VLAN_ID_MIN && vid <= VTSS_APPL_VLAN_ID_MAX));

    vls.set(vid);
}

void mgmt_types_vlan_list_bit_value_clear(const vtss_vlan_list_t *const vlan_list, const mesa_vid_t vid)
{
    vtss::VlanList     &vls = (vtss::VlanList &)(*vlan_list);

    VTSS_ASSERT(NULL != vlan_list && (vid >= VTSS_APPL_VLAN_ID_MIN && vid <= VTSS_APPL_VLAN_ID_MAX));

    vls.clear(vid);
}

/* Output the list of VLAN ID, e.g. 1,3-5,7  */
std::string mgmt_vid_list_to_txt(const vtss::Set<mesa_vid_t> &vid_list) {
    vtss::StringStream str_buf;
    bool is_continuity = false; /* If the digit number is continuous or not */
    size_t cnt = 0;

    for (auto itr = vid_list.begin(), prev_itr = vid_list.end();
         itr != vid_list.end(); prev_itr = itr, ++itr, ++cnt) {
        if (itr == vid_list.begin()) {
            str_buf << *itr;
            continue;
        }

        is_continuity = (*itr == (*prev_itr + 1));

        if (is_continuity && cnt != (vid_list.size() - 1)) {
            // The digit number is continuous and it is not the last iterator
            continue;
        }

        str_buf << (is_continuity ? "-" : ",") << *itr;
        is_continuity = false;
    }

    return str_buf.buf;
}

/******************************************************************************/
// mgmt_port_list_to_stackable()
// This function is not stack-aware!
/******************************************************************************/
void mgmt_port_list_to_stackable(vtss_port_list_stackable_t &stackable, const mesa_port_list_t &port_list)
{
    vtss::PortListStackable &pls = (vtss::PortListStackable &)stackable;
    mesa_port_no_t          port_no;
    uint32_t                port_cnt = port_count_max();

    vtss_clear(stackable);

    for (port_no = 0; port_no < port_cnt; port_no++) {
        if (port_list[port_no]) {
            pls.set(VTSS_ISID_START, port_no);
        }
    }
}

/******************************************************************************/
// mgmt_port_list_from_stackable()
// This function is not stack-aware!
/******************************************************************************/
void mgmt_port_list_from_stackable(const vtss_port_list_stackable_t &stackable, mesa_port_list_t &port_list)
{
    const vtss::PortListStackable &pls = (vtss::PortListStackable &)stackable;
    mesa_port_no_t                port_no;
    uint32_t                      port_cnt = port_count_max();

    vtss_clear(port_list);

    for (port_no = 0; port_no < port_cnt; port_no++) {
        port_list[port_no] = pls.get(VTSS_ISID_START, port_no);
    }
}

