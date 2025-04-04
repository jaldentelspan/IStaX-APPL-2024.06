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

/*
==============================================================================

    utility for ICLI engine

    Revision history
    > CP.Wang, 05/29/2013 13:09
        - create

==============================================================================
*/
#ifndef __VTSS_ICLI_UTIL__
#define __VTSS_ICLI_UTIL__
//****************************************************************************

/*
==============================================================================

    Type Definition

==============================================================================
*/
typedef enum {
    ICLI_IPV4_CLASS_A,
    ICLI_IPV4_CLASS_B,
    ICLI_IPV4_CLASS_C,
    ICLI_IPV4_CLASS_D,
    ICLI_IPV4_CLASS_E,
} icli_ipv4_class_t;

/*
==============================================================================

    Public Function

==============================================================================
*/
#ifdef __cplusplus
extern "C" {
#endif

/*
    count the number of words in a command string.
    without ID and ID Delimiter

    INPUT
        cmd : command string

    OUTPUT
        n/a

    RETURN
        >=0: number of words in command string
        <0 : failed, icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_word_count(
    IN  char *cmd
);

/*
    get next word, keyword or variable

    INPUT
        cmd : command string

    OUTPUT
        cmd : the string behind the word got

    RETURN
        char * : word
        NULL   : no word

    COMMENT
        cmd will be damaged.
*/
char *vtss_icli_word_get_next(
    INOUT  char **cmd
);

/*
    convert the string into upper case

    INPUT
        s : string

    OUTPUT
        s : converted string

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_to_upper_case(
    INOUT char  *s
);

/*
    convert the string to lower case

    INPUT
        s : string

    OUTPUT
        s : converted string

    RETURN
        icli_rc_t

    COMMENT
        n/a
*/
i32 vtss_icli_to_lower_case(
    INOUT char  *s
);

/*
    get the length of string

    INPUT
        str : string

    OUTPUT
        n/a

    RETURN
        length of string
*/
u32 vtss_icli_str_len(
    IN  const char  *str
);

/*
    copy string from src to dst

    INPUT
        src : source string

    OUTPUT
        dst : destined string

    RETURN
        dst  : successful
        NULL : dst or src is NULL
*/
char *vtss_icli_str_cpy(
    OUT char        *dst,
    IN  const char  *src
);

/*
    copy string from src to dst
    the max number(cnt) of char's to be copied

    INPUT
        src : source string
        cnt : number of char copied

    OUTPUT
        dst : destined string
              dst's buffer length should be more than cnt

    RETURN
        dst  : successful
        NULL : dst or src is NULL, or cnt is 0
*/
char *vtss_icli_str_ncpy(
    OUT char            *dst,
    IN  const char      *src,
    IN  u32             cnt
);

/*
    compare two strings, s1 and s2
    case sensitive

    INPUT
        s1 : string 1
        s2 : string 2

    OUTPUT
        n/a

    RETURN
         1 : s1 > s2
         0 : s1 == s2
        -1 : s1 < s2
        -2 : parameter error
*/
i32 vtss_icli_str_cmp(
    IN  const char  *s1,
    IN  const char  *s2
);

/*
    check if sub is a sub-string of s

    INPUT
        sub : sub-string
        s   : string
        case_sensitive : check in case-sensitive or not
                         0 : case-insensitive
                         1 : case-sensitive

    OUTPUT
        err_pos : error position start from 0
                  work when it is present

    RETURN
         1 : sub-string
         0 : exactly match
        -1 : not even sub
*/
i32 vtss_icli_str_sub(
    IN  const char  *sub,
    IN  const char  *s,
    IN  u32         case_sensitive,
    OUT u32         *err_pos
);

/*
    concatenate src to the end of dst

    INPUT
        src : source string

    OUTPUT
        dst : destined string

    RETURN
        dst  : successful
        NULL : dst or src is NULL
*/
char *vtss_icli_str_concat(
    OUT char        *dst,
    IN  const char  *src
);

/*
    find if sub is a substring IN s

    INPUT
        sub : substring
        s   : string

    OUTPUT
        none

    RETURN
        char* : start ptr in dst for src
        NULL  : sub or s is NULL or fail to find
*/
char *vtss_icli_str_str(
    IN  const char   *sub,
    IN  const char   *s
);

/*
    IPv4 address: u32 to string

    INPUT
        ip : IPv4 address

    OUTPUT
        s : its buffer size must be at least 16 bytes,
            output string in the format of xxx.xxx.xxx.xxx

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_ipv4_to_str(
    IN  mesa_ip_t   ip,
    OUT char        *s
);

/*
    get address class by IP

    INPUT
        ip : IPv4 address

    OUTPUT
        n/a

    RETURN
        icli_ipv4_class_t
*/
icli_ipv4_class_t vtss_icli_ipv4_class_get(
    IN  mesa_ip_t   ip
);

/*
    get netmask by prefix

    INPUT
        prefix : 0 - 32

    OUTPUT
        netmask : 0.0.0.0 - 255.255.255.255

    RETURN
        TRUE  - successful
        FALSE - failed
*/
BOOL vtss_icli_ipv4_prefix_to_netmask(
    IN  u32         prefix,
    OUT mesa_ip_t   *netmask
);

/*
    get prefix by netmask

    INPUT
        netmask : 0.0.0.0 - 255.255.255.255

    OUTPUT
        prefix : 0 - 32

    RETURN
        TRUE  - successful
        FALSE - failed
*/
BOOL vtss_icli_ipv4_netmask_to_prefix(
    IN   mesa_ip_t   netmask,
    OUT  u32         *prefix
);

/*
    get prefix by IPv6 netmask

    INPUT
        netmask : :: - FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF

    OUTPUT
        prefix : 0 - 128

    RETURN
        TRUE  - successful
        FALSE - failed
*/
BOOL vtss_icli_ipv6_netmask_to_prefix(
    IN   mesa_ipv6_t    *netmask,
    OUT  u32            *prefix
);

/*
    IPv6 address: mesa_ipv6_t to string

    INPUT
        ipv6 : IPv6 address

    OUTPUT
        s : its buffer size MUST be larger than 40 bytes,
            output string in the format of hhhh:hhhh:hhhh:hhhh:hhhh:hhhh:hhhh:hhhh

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_ipv6_to_str(
    IN  mesa_ipv6_t     ipv6,
    OUT char            *s
);

/*
    MAC address: u8 [6] to string

    INPUT
        mac : MAC address

    OUTPUT
        s : its buffer size must be at least 18 bytes,
            output string in the format of xx-xx-xx-xx-xx-xx

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_mac_to_str(
    IN  u8     *mac,
    OUT char    *s
);

/*
    convert a string to an integer

    INPUT
        s : the string to be converted

    OUTPUT
        val : signed integer value

    RETURN
         0 : successful
        -1 : failed
*/
i32 vtss_icli_str_to_int(
    IN  const char  *s,
    OUT i32         *val
);

/*
    find the number of the same prefix chars between 2 strings

    INPUT
        s1  : string 1
        s2  : string 2
        len : max length for check
              if 0 then no limit
        cs  : check in case-sensitive or not
              0 : case-insensitive
              1 : case-sensitive

    OUTPUT
        n/a

    RETURN
         >0 : the number of char's for the same prefix
         =0 : no same prefix
*/
u32 vtss_icli_str_prefix(
    IN  const char  *s1,
    IN  const char  *s2,
    IN  u32         len,
    IN  u32         cs
);

/*
    Switch port range to string

    INPUT
        spr : switch port range

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_switch_port_range_to_str(
    IN  icli_switch_port_range_t    *spr,
    OUT char                        *s
);

/*
    Stack port range to string

    INPUT
        spr : switch port range

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_stack_port_range_to_str(
    IN  icli_stack_port_range_t     *spr,
    OUT char                        *s
);

/*
    Unsigned range to string

    INPUT
        ur : unsigned range

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_unsigned_range_to_str(
    IN  icli_unsigned_range_t   *ur,
    OUT char                    *s
);

/*
    Signed range to string

    INPUT
        sr : signed range

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_signed_range_to_str(
    IN  icli_signed_range_t     *sr,
    OUT char                    *s
);

/*
    Range to string

    INPUT
        r : range

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_range_to_str(
    IN  icli_range_t    *r,
    OUT char            *s
);

/*
    OUI to string

    INPUT
        oui : OUI

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_oui_to_str(
    IN  icli_oui_t      *oui,
    OUT char            *s
);

/*
    Clock ID to string

    INPUT
        clock_id : Clock ID

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_clock_id_to_str(
    IN  icli_clock_id_t     *clock_id,
    OUT char                *s
);

/*
    VCAP VR to string

    INPUT
        vcap_vr : VCAP VR

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_vcap_vr_to_str(
    IN  icli_vcap_vr_t      *vcap_vr,
    OUT char                *s
);

/*
    Hex array to string

    INPUT
        hexval : hex array

    OUTPUT
        s : its buffer size must be large enough

    RETURN
        s    : successful
        NULL : failed
*/
char *vtss_icli_hexval_to_str(
    IN  icli_hexval_t       *hexval,
    OUT char                *s
);

/*
    A valid file name is a text string
    1. alphabet(A-Za-z), digits(0-9), dot(.), dash(-), under line(_).
    2. The maximum length is 64.
    3. dash(-) must not be first character.
    4. The file name can not contains only dot(.).
       For example, ".", ".." and "....." are not allowed.
*/
i32 vtss_icli_file_name_check(
    IN  char    *file_name,
    OUT u32     *err_pos
);

#ifdef __cplusplus
}
#endif

//****************************************************************************
#endif //__VTSS_ICLI_UTIL__
