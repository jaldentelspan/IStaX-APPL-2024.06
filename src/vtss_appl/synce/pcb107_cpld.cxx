/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "microchip/ethernet/switch/api.h"
#include "misc_api.h"
#include "synce_trace.h"

#include "pcb107_cpld.h"
#include "synce_spi_if.h"
#include <dirent.h>

/****************************************************************************/
/* Currently, PCB-135 Cpld SPI APIs are also added to this file. All the    */
/* contents of this file need to be moved to MEBA in future.                */
/****************************************************************************/

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

/* CPLD register defines */
#define PCB107_CPLD_ID      0x01      // Cpld ID
#define PCB107_CPLD_REV     0x02      // Cpld Revision
#define PCB107_CPLD_MUX0    0x06      // MUX 0
#define PCB107_CPLD_MUX1    0x07      // MUX 1
#define PCB107_CPLD_MUX2    0x08      // MUX 2
#define PCB107_CPLD_MUX3    0x09      // MUX 3

#define PCB107_CPLD_MUX_MAX 0x04      // Number of muxes
#define PCB107_CPLD_MUX_INPUT_MAX 20  // number of mux inputs
#define VTSS_PHY_TS_FIFO_EMPTY    -1

#define VTSS_TS_SPI_FIFO_EMPTY   0x01
#define VTSS_TS_SPI_FIFO_DROP    0x02
#define VTSS_TS_SPI_FIFO_FULL    0x04

/* PCB-135 cpld constants */
#define PCB135_CPLD_MUX0    0x01      // MUX 0
#define PCB135_CPLD_MUX1    0x02      // MUX 1
#define PCB135_CPLD_MUX2    0x03      // MUX 2
#define PCB135_CPLD_MUX3    0x04      // MUX 3

#define PCB135_CPLD_MUX_MAX 0x04      // Number of muxes
#define PCB135_CPLD_MUX_INPUT_MAX 31  // number of mux inputs


static synce_cpld_spi_if dev_pcb_107_cpld;
static synce_cpld_spi_if dev_pcb_107_cpld_tod;
static synce_cpld_spi_if dev_pcb_135_cpld;

/* Lengths of the signature fields */
#define VTSS_PHY_TS_SIG_TIME_STAMP_LEN     10
#define VTSS_PHY_TS_SIG_SOURCE_PORT_ID_LEN 10
#define VTSS_PHY_TS_SIG_SEQUENCE_ID_LEN    2
#define VTSS_PHY_TS_SIG_DEST_IP_LEN        4
#define VTSS_PHY_TS_SIG_SRC_IP_LEN         4
#define VTSS_PHY_TS_SIG_DEST_MAC_LEN       6
#define VTSS_PHY_SPI_TS_POS                16
#define TS_FIFO_CLEAR_VALUE              0xDE
#define TS_SPI_STATUS_POS           5
#define VTSS_SPI_VALID_TS           64

#define VTSS_BIT_SHIFT(x) ((x) << TS_FPGA_SPI_MUX_POS)
#define VTSS_BIT_CLEAR(x) ((~0) << (x))

#define VTSS_TS_FPGA_ASSERT(x) if((x)) { return MESA_RC_ERROR;}

#define FIFO_EMPTY            0x01
#define FIFO_ALMOSTEMPTY      0x02
#define FIFOALMOSTFULL        0x04
#define FIFO_FULL             0x08
#define FIFO_DROP             0x10
#define FIFO_START_TS         0x20


void pcb107_cpld_write(uchar address, uchar data)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        dev_pcb_107_cpld.write(address, data);
    } else {
        T_EG(TRACE_GRP_API, "PCB107 CPLD not supported");
    }
}

void pcb107_cpld_read(uchar address, uchar *data)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        dev_pcb_107_cpld.read(address, data);
    } else {
        T_EG(TRACE_GRP_API, "PCB107 CPLD not supported");
    }
}

static void pcb107_cpld_tod_fifo_clear()
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        u8 tx_data, rx_data;
        dev_pcb_107_cpld_tod.spi_transfer(1, &tx_data, &rx_data);
    } else {
        T_EG(TRACE_GRP_API, "PCB107 CPLD not supported");
    }
}

static uchar cpld_id;
static uchar cpld_rev;

void pcb107_cpld_init(void)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        if (meba_capability(board_instance, MEBA_CAP_PCB107_CPLD_CS_VIA_MUX)) {
            char cpld_spi_file[32];
            if (misc_find_spidev(cpld_spi_file, sizeof(cpld_spi_file), "synce_builtin\n") == MESA_RC_OK) {
                dev_pcb_107_cpld.init(cpld_spi_file);
                dev_pcb_107_cpld_tod.init(cpld_spi_file);
            } else {
                T_EG(TRACE_GRP_API, "PCB107 CPLD could not be found");
            }
        } else {
            char cpld_spi_file[32];
            char cpld_fifo_spi_file[32];
            if (misc_find_spidev(cpld_spi_file, sizeof(cpld_spi_file), "synce_builtin\n") == MESA_RC_OK && misc_find_spidev(cpld_fifo_spi_file, sizeof(cpld_fifo_spi_file), "cpld_fifo\n") == MESA_RC_OK ) {
                dev_pcb_107_cpld.init(cpld_fifo_spi_file);
                dev_pcb_107_cpld_tod.init(cpld_spi_file);
            } else {
                T_EG(TRACE_GRP_API, "PCB107 CPLD could not be found");
            }
        }
        pcb107_cpld_read(PCB107_CPLD_ID, &cpld_id);
        pcb107_cpld_read(PCB107_CPLD_REV, &cpld_rev);
        T_IG(TRACE_GRP_API, "PCB107 CPLD id %d, revision %d", cpld_id, cpld_rev);
    } else {
        T_EG(TRACE_GRP_API, "PCB107 CPLD not supported");
    }
}

static u32 curr_mux [PCB107_CPLD_MUX_MAX] = {0xff,0xff,0xff,0xff};

void pcb107_cpld_mux_set(u32 mux, u32 input)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        uchar mux_reg;
        if (mux >= PCB107_CPLD_MUX_MAX || input >= PCB107_CPLD_MUX_INPUT_MAX) {
            T_EG(TRACE_GRP_API, "Invalid mux %d or input %d", mux, input);
        } else {
            if (input != curr_mux[mux]) {
                mux_reg = (cpld_rev > 1) ? PCB107_CPLD_MUX0 + mux : PCB107_CPLD_MUX3 - mux;
                T_WG(TRACE_GRP_API, "Setting mux %d to input %d", mux, input);
                pcb107_cpld_write(mux_reg, input);
                curr_mux[mux] = input;
            }
        }
    } else {
        T_IG(TRACE_GRP_API, "PCB107 CPLD not supported on this board");
    }
}

void pcb107_fifo_clear(void)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        pcb107_cpld_read(PCB107_CPLD_ID, &cpld_id);
        pcb107_cpld_read(PCB107_CPLD_REV, &cpld_rev);
        T_IG(TRACE_GRP_API, "PCB107 CPLD id %d, revision %d", cpld_id, cpld_rev);
        pcb107_cpld_tod_fifo_clear();
    } else {
        T_IG(TRACE_GRP_API, "PCB107 CPLD not supported on this board");   
    }
}
mesa_rc vtss_ts_spi_fpga_read(vtss_phy_timestamp_t      *const ts,
                                vtss_phy_ts_fifo_sig_t  *const signature,
                                int                     *const status,
                                u16                     *const channel_no,
                                const u16               port_no)
{
    if (!meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB107_CPLD)) {
        T_IG(TRACE_GRP_API, "PCB107 CPLD not supported on this board");   
        return MESA_RC_ERROR;
    }
    
    u8                          sig[26], first = 0;
    int                         loop_cnt, pos, sig_size = 0;
    vtss_phy_ts_fifo_sig_mask_t sig_mask;
    u8 value;
    memset(signature, 0, sizeof(vtss_phy_ts_fifo_sig_t));
    memset(ts, 0, sizeof(vtss_phy_timestamp_t));
/*
To read from the FIFO, use the following sequence:
1. Read FIFO status register FRnSTAT. If the FIFO is not empty, goto 2. The FIFO_START_TS bit must be one if this is the first byte of a timestamp.
2. Read the current byte, and advance the FIFO using the FRnN register. Repeat as many times are there are bytes in the timestamp.
3. Optionally read the status register between each FRnN read. Goto 1.*/
    while(1) {
        loop_cnt = 0;
        sig[loop_cnt] = 0;
        value  = 0;
        pcb107_cpld_read(0x23, &value);
        T_IG(TRACE_GRP_API, "value::%x", value);
        if ((value & FIFO_EMPTY) == 0) {        /*check for FIFO_EMPTY*/
            if((value & FIFO_ALMOSTEMPTY) == FIFO_ALMOSTEMPTY) {/*Check for FIFO_ALMOSTEMPTY*/
                continue;
            } else {
                if ((value & 0x20) == 0) {
                    int i;
                    T_I("FIFO head is not at first byte of TS");
                    for(i=0;i < 27;i++) {
                        first = 0;
                        pcb107_cpld_read(0x21, &first);
                        T_I("first::%x", first);
                        value = 0;
                        pcb107_cpld_read(0x23, &value);
                        T_I("value_1::%x", value);
                        if ((value & 0x20) != 0)
                            break;
                    }
                    if (i>27)  {
                       T_E("Not able to reach the first byte of TS");
                       return MESA_RC_ERROR;
                    }
                    continue;
                } else if ((value & FIFO_START_TS) != 0) {
                    first = 0;
                    pcb107_cpld_read(0x21, &first);
                    T_I("first_1::%x", value);

                    /* Additional Checks added as per Hardware requirements*/
                    if ((first & VTSS_BIT_CLEAR(TS_SPI_STATUS_POS)) != VTSS_SPI_VALID_TS) {
                        T_E("Returning here as 1st 3 bits are not 0b100");
                        *status = VTSS_PHY_TS_FIFO_EMPTY;
                        return MESA_RC_ERROR;
                    }
                    loop_cnt = 0;
                    VTSS_OS_MSLEEP(30);
                    while (loop_cnt != 26) {
                        pcb107_cpld_read(0x21, &sig[loop_cnt]);
                        T_IG(TRACE_GRP_API, " %2X ", sig[loop_cnt]);
                        loop_cnt++;
                    }
                    break;
                }
            }
        } else {
            *status = VTSS_PHY_TS_FIFO_EMPTY;
            return MESA_RC_ERROR;
        }
    }
    pos = 0;
    *channel_no = first & 0x1F;
    VTSS_TS_FPGA_ASSERT(vtss_phy_ts_fifo_sig_get(NULL, port_no, &sig_mask));
    signature->sig_mask = sig_mask;
    pos = 0;/* In case of reading through SPI TS is followed by the signature So reading the signature first*/

    /* All zeros before the signaure */
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SRC_IP) {
       sig_size += VTSS_PHY_TS_SIG_SRC_IP_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_IP) {
       sig_size += VTSS_PHY_TS_SIG_DEST_IP_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_MSG_TYPE) {
       sig_size ++;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DOMAIN_NUM) {
       sig_size ++;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SOURCE_PORT_ID) {
       sig_size += VTSS_PHY_TS_SIG_SOURCE_PORT_ID_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SEQ_ID) {
        sig_size += VTSS_PHY_TS_SIG_SEQUENCE_ID_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_MAC) {
        sig_size += VTSS_PHY_TS_SIG_DEST_MAC_LEN;
    }
    pos = 16 - sig_size;
    /* All zeros before the signaure */

    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SRC_IP) {
        VTSS_TS_FPGA_ASSERT((pos + VTSS_PHY_TS_SIG_SRC_IP_LEN) > 26); /* LINT */
        signature->src_ip = (sig[pos + 3] << 24) | (sig[pos + 2] << 16) | (sig[pos + 1] << 8) | sig[pos];
        pos += VTSS_PHY_TS_SIG_SRC_IP_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_IP) {
        VTSS_TS_FPGA_ASSERT((pos + VTSS_PHY_TS_SIG_DEST_IP_LEN) > 26); /* LINT */
        signature->dest_ip = (sig[pos + 3] << 24) | (sig[pos + 2] << 16) | (sig[pos + 1] << 8) | sig[pos];
        pos += VTSS_PHY_TS_SIG_DEST_IP_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_MSG_TYPE) {
    /* message_type field is only the lower nibble
                */
        VTSS_TS_FPGA_ASSERT((pos + 1) > 26); /* LINT */
        signature->msg_type = sig[pos++] & 0x0f;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DOMAIN_NUM) {
        VTSS_TS_FPGA_ASSERT((pos + 1) > 26); /* LINT */
        signature->domain_num = sig[pos++];
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SOURCE_PORT_ID) {
        VTSS_TS_FPGA_ASSERT((pos + VTSS_PHY_TS_SIG_SOURCE_PORT_ID_LEN) > 26); /* LINT */
        for(loop_cnt = 0; loop_cnt <= 9; loop_cnt++) { /* 0 - 9, Total 10 Byte Source Port ID */
            signature->src_port_identity[loop_cnt] = sig[pos + loop_cnt];
        }
            pos += VTSS_PHY_TS_SIG_SOURCE_PORT_ID_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_MAC) {
        VTSS_TS_FPGA_ASSERT((pos + VTSS_PHY_TS_SIG_DEST_MAC_LEN) > 26);
        for(loop_cnt = 5; loop_cnt >= 0; loop_cnt--) {
            signature->dest_mac[loop_cnt] = sig[pos + loop_cnt];
        }
             pos += VTSS_PHY_TS_SIG_DEST_MAC_LEN;
    }
    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SEQ_ID) {
        VTSS_TS_FPGA_ASSERT((pos + VTSS_PHY_TS_SIG_SEQUENCE_ID_LEN) > 26); /* LINT */
        signature->sequence_id = (sig[pos] << 8) | sig[pos + 1];
        pos += VTSS_PHY_TS_SIG_SEQUENCE_ID_LEN;
    }
    ts->seconds.high = (sig[pos] << 8) | sig[pos + 1];
    ts->seconds.low = (sig[pos + 2] << 24) | (sig[pos + 3] << 16) | (sig[pos + 4] << 8)  | sig[pos + 5];
    ts->nanoseconds = (sig[pos + 6] << 24) | (sig[pos + 7] << 16) | (sig[pos + 8] << 8)  | sig[pos + 9];
    *status = VTSS_PHY_TS_FIFO_SUCCESS;

    return MESA_RC_OK;
}

/* pcb135 cpld */
void pcb135_cpld_write(u8 address, u8 data)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
        dev_pcb_135_cpld.write(address, data);
    } else {
        T_IG(TRACE_GRP_API, "PCB135 CPLD not supported");
    }
}

void pcb135_cpld_read(u8 address, u8 *data)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
        dev_pcb_135_cpld.read(address, data);
    } else {
        T_IG(TRACE_GRP_API, "PCB135 CPLD not supported");
    }
}

void pcb135_cpld_init(void)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
        char cpld_spi_file[32];
        if (misc_find_spidev(cpld_spi_file, sizeof(cpld_spi_file), "cpld\n") == MESA_RC_OK) {
            dev_pcb_135_cpld.init(cpld_spi_file);
            for (int mux_reg = PCB135_CPLD_MUX0; mux_reg <= PCB135_CPLD_MUX_MAX; mux_reg++) {
                pcb135_cpld_write(mux_reg, 0);
            }
        } else {
            T_IG(TRACE_GRP_API, "PCB135 CPLD could not be found");
        }
    }
}

void pcb135_cpld_mux_set(u32 mux, u32 input)
{
    if (meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
        u8 mux_reg;
        if (mux >= PCB135_CPLD_MUX_MAX || input >= PCB135_CPLD_MUX_INPUT_MAX) {
            T_EG(TRACE_GRP_API, "Invalid mux %d or input %d", mux, input);
        } else {
            if (input != curr_mux[mux]) {
                mux_reg = PCB135_CPLD_MUX0 + mux;
                T_WG(TRACE_GRP_API, "Setting mux %d to input %d", mux, input);
                pcb135_cpld_write(mux_reg, input);
                curr_mux[mux] = input;
            }
        }
    } else {
        T_IG(TRACE_GRP_API, "PCB107 CPLD not supported on this board");
    }
}

