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
#include <string.h>
#include <vtss/basics/trace.hxx>
#include <vtss/basics/types.hxx>
#include <vtss/basics/formatting_tags.hxx>

void some_value(mesa_ip_network_t *v) {
    memset(v, 0, sizeof(*v));
    v->address.type = MESA_IP_TYPE_IPV6;
    v->address.addr.ipv6.addr[0] = 0x20;
    v->address.addr.ipv6.addr[1] = 0x01;
    v->address.addr.ipv6.addr[15] = 1;
    v->prefix_size = 112;
}

int main() {
    using namespace vtss;
    uint64_t big_number = -1ul;

    // A simple text traces
    VTSS_TRACE(INFO) << "A normal trace message, new lines are added automatic";

    // Tracing using different levels
    VTSS_TRACE(NOISE)   << "This is a noise trace";
    VTSS_TRACE(DEBUG)   << "This is a debug trace";
    VTSS_TRACE(INFO)    << "This is a info trace";
    VTSS_TRACE(WARNING) << "This is a warning trace";
    VTSS_TRACE(ERROR)   << "This is a error trace";
    VTSS_TRACE(FATAL)   << "This is a fatal trace";

    // Tracing numbers
    VTSS_TRACE(INFO) << "This is how to trace a number: " << 42 <<
        " or you may of cause us a variable: " << big_number;

    mesa_ip_network_t network;

    some_value(&network);
    VTSS_TRACE(INFO) << "This is a network address: " << network;


    mesa_ipv4_network_t i = {0xa000001, 24};
    VTSS_TRACE(INFO) << "i: " << i;

    uint32_t j = 42, x = 1000;
    VTSS_TRACE(INFO) << "i: " << HEX(j);
    VTSS_TRACE(INFO) << "i: " << hex(j);
    VTSS_TRACE(INFO) << "i: " << hex_fixed<4>(j);
    VTSS_TRACE(INFO) << "| " << right<5>(j) << " <-> " << left<5>(x) << " |";
    VTSS_TRACE(INFO) << "| " << right<5>(x) << " <-> " << left<5>(j) << " |";

    mesa_ipv4_t ipv4 = 0xa000001;
    uint32_t my_bool = true;
    uint64_t us = 0x123456789abce;
    VTSS_TRACE(INFO) << "IP: " << AsIpv4(ipv4) << " as int: " << ipv4;
    VTSS_TRACE(INFO) << "Bool: " << AsBool(my_bool) << " as int: " << my_bool;
    VTSS_TRACE(INFO) << "us: " << as_time_us(us);

    return 0;
}

