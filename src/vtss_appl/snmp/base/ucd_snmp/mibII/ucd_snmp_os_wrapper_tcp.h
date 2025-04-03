/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

typedef struct {
    u_long   tcpRtoAlgorithm;
    u_long   tcpRtoMin;
    u_long   tcpRtoMax;
    u_long   tcpMaxConn;
    u_long   tcpActiveOpens;
    u_long   tcpPassiveOpens;
    u_long   tcpAttemptFails;
    u_long   tcpEstabResets;
    u_long   tcpCurrEstab;
    u_long   tcpInSegs;
    u_long   tcpOutSegs;
    u_long   tcpRetransSegs;
    u_long   tcpInErrs;
    u_long   tcpOutRsts;
    u_short  tcpInErrsValid;
    u_short  tcpOutRstsValid;
} tcp_stat_scalar_t;

typedef struct {
    u_char   tcpConnectionLocalAddress[4];
    uint32_t   tcpConnectionLocalPort;
    u_char   tcpConnectionRemAddress[4];
    uint32_t   tcpConnectionRemPort;
    u_long   tcpConnectionState;
} tcp_connTable_entry_t;

enum {
        TCP_ESTABLISHED = 1,
        TCP_SYN_SENT,
        TCP_SYN_RECV,
        TCP_FIN_WAIT1,
        TCP_FIN_WAIT2,
        TCP_TIME_WAIT,
        TCP_CLOSE,
        TCP_CLOSE_WAIT,
        TCP_LAST_ACK,
        TCP_LISTEN,
        TCP_CLOSING,    /* Now a valid state */
        TCP_NEW_SYN_RECV,
        TCP_MAX_STATES  /* Leave at the end! */
};
u_long linux_tcp_stat_get(tcp_stat_scalar_t *tcp_stat);
u_long linux_tcp_conn_entry_iterator(void **pre, tcp_connTable_entry_t *entry);
#define os_wrapper_tcp_stat_get linux_tcp_stat_get
#define os_wrapper_tcp_conn_entry_iterator linux_tcp_conn_entry_iterator
