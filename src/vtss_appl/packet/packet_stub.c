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

#include "packet_api.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PACKET

mesa_rc packet_init(vtss_init_data_t *data)
{
    return VTSS_RC_OK;
}

/******************************************************************************/
// packet_tx()
// Inject frame.
/******************************************************************************/
mesa_rc packet_tx(packet_tx_props_t *tx_props)
{
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// packet_tx_alloc()
// Size argument should not include IFH, CMD, and FCS
/******************************************************************************/
unsigned char *packet_tx_alloc(size_t size)
{
    printf("packet_tx_alloc.2 packet_stub.c\n");
    return VTSS_MALLOC(size);
}

/******************************************************************************/
/******************************************************************************/
void packet_tx_free(unsigned char *buffer)
{
    VTSS_FREE(buffer);
}

/******************************************************************************/
// ip_stack_glue_set_aggr_code_enable()
// API function to enable and disable computation of aggregation codes.
/******************************************************************************/
void packet_tx_aggr_code_enable(BOOL sipdip_ena, BOOL tcpudp_ena)
{
}

mesa_rc packet_rx_filter_register(const packet_rx_filter_t *filter, void **filter_id)
{
    return VTSS_RC_OK;
}

void packet_rx_filter_init(packet_rx_filter_t *filter)
{
}

void packet_dbg(packet_dbg_printf_t dbg_printf, ulong parms_cnt, ulong *parms)
{
}

void packet_ipv4_set(mesa_ipv4_t ipv4_addr)
{
}

unsigned char *packet_tx_alloc_extra(size_t size, size_t extra_size_dwords, unsigned char **extra_ptr)
{
    unsigned char *buffer;
    size_t extra_size_bytes = 4 * extra_size_dwords;
    if ((buffer = VTSS_MALLOC(size + extra_size_bytes))) {
        *extra_ptr = buffer;
        buffer += extra_size_bytes;
    }
    return buffer;
}

void packet_tx_free_extra(unsigned char *extra_ptr)
{
    VTSS_FREE(extra_ptr);
}

mesa_rc packet_rx_filter_change(const packet_rx_filter_t *filter, void **filter_id)
{
    return PACKET_ERROR_GEN;
}

mesa_rc packet_rx_filter_unregister(void *filter_id)
{
    return PACKET_ERROR_GEN;
}

void packet_rx_no_arp_discard_add_ref(mesa_port_no_t port)
{
}

void packet_rx_no_arp_discard_release(mesa_port_no_t port)
{
}

void packet_tx_props_init(packet_tx_props_t *tx_props)
{
}

