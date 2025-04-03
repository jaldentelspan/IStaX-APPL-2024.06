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

#include "mac_utils.hxx"

mesa_mac_t mac_broadcast = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

/******************************************************************************/
// mesa_mac_t::operator<()
/******************************************************************************/
bool operator<(const mesa_mac_t &a, const mesa_mac_t &b)
{
    return (a.addr[0] != b.addr[0] ? (a.addr[0] < b.addr[0]) :
            a.addr[1] != b.addr[1] ? (a.addr[1] < b.addr[1]) :
            a.addr[2] != b.addr[2] ? (a.addr[2] < b.addr[2]) :
            a.addr[3] != b.addr[3] ? (a.addr[3] < b.addr[3]) :
            a.addr[4] != b.addr[4] ? (a.addr[4] < b.addr[4]) :
            (a.addr[5] < b.addr[5]));
}

/******************************************************************************/
// mesa_mac_t::operator!=()
/******************************************************************************/
bool operator!=(const mesa_mac_t &a, const mesa_mac_t &b)
{
    return (a.addr[0] != b.addr[0] ||
            a.addr[1] != b.addr[1] ||
            a.addr[2] != b.addr[2] ||
            a.addr[3] != b.addr[3] ||
            a.addr[4] != b.addr[4] ||
            a.addr[5] != b.addr[5]);
}

/******************************************************************************/
// mesa_mac_t::operator==()
/******************************************************************************/
bool operator==(const mesa_mac_t &a, const mesa_mac_t &b)
{
    return (a.addr[0] == b.addr[0] &&
            a.addr[1] == b.addr[1] &&
            a.addr[2] == b.addr[2] &&
            a.addr[3] == b.addr[3] &&
            a.addr[4] == b.addr[4] &&
            a.addr[5] == b.addr[5]);
}

/******************************************************************************/
// mesa_mac_t::operator&=()
/******************************************************************************/
mesa_mac_t &operator&=(mesa_mac_t &a, const mesa_mac_t &b)
{
    int i;
    for (i = 0; i < ARRSZ(a.addr); i++) {
        a.addr[i] &= b.addr[i];
    }

    return a;
}

/******************************************************************************/
// mesa_vid_mac_t::operator<()
/******************************************************************************/
bool operator<(const mesa_vid_mac_t &a, const mesa_vid_mac_t &b)
{
    if (a.vid != b.vid) {
        return a.vid < b.vid;
    }

    return a.mac < b.mac;
}

/******************************************************************************/
// mac_is_unicast()
/******************************************************************************/
bool mac_is_unicast(const mesa_mac_t &m)
{
    return (m.addr[0] & 0x1) == 0;
}

/******************************************************************************/
// mac_is_multicast()
/******************************************************************************/
bool mac_is_multicast(const mesa_mac_t &m)
{
    return !mac_is_unicast(m);
}

/******************************************************************************/
// mac_is_broadcast()
/******************************************************************************/
bool mac_is_broadcast(const mesa_mac_t &m)
{
     return m == mac_broadcast;
}

/******************************************************************************/
// mac_is_zero()
/******************************************************************************/
bool mac_is_zero(const mesa_mac_t &m)
{
    static mesa_mac_t all_zeroes_mac;
    return m == all_zeroes_mac;
}

/******************************************************************************/
// mesa_vid_mac_t::operator<<()
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_vid_mac_t &i)
{
    o << "{vid:" << i.vid << " mac:" << i.mac << "}";
    return o;
}

/******************************************************************************/
// fmt(mesa_vid_mac_t);
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_vid_mac_t *r)
{
    o << *r;
    return 0;
}

