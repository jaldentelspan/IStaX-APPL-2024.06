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

#ifdef VTSS_SW_OPTION_PIC

#include "main.h"
#include "board_misc.h"
#include "board_if.h"
#include "led_api.h"

#include <cyg/hal/plf_io.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_if.h>
#define DELAY_US(_usdelay_) CYGACC_CALL_IF_DELAY_US (_usdelay_)

#include "pic_serval_data.h"

//#define PIC_DEBUG
#ifdef PIC_DEBUG
#define PIC_TRACE(format, ...) diag_printf(format, ## __VA_ARGS__)
#else
#define PIC_TRACE(format, ...)
#endif
#define PIC_NOTICE(format, ...) diag_printf(format, ## __VA_ARGS__)

/* This is for offsets 0x300000 and forward */
/*                            00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D */
static u8 config_mask[] = { 0x00, 0xCF, 0x1F, 0x1F, 0x00, 0x8F, 0xC5, 0x00, 0x0F, 0xC0, 0x0F, 0xE0, 0x0F, 0x40};

/* 
 *
 * Note - this code assumes:
 *
 * PGM:  GPIO4
 * MCLR: GPIO5
 * PGC:  GPIO11
 * PGD:  GPIO12
 *
 * This is currently available on the XXXXXXX board.
 */

#define GPIO_PGM   4
#define GPIO_MCLR  5
#define GPIO_PGC  11
#define GPIO_PGD  12

#define CMD_CORE            0x0  /* 0b'0000' */
#define CMD_TABLAT          0x2  /* 0b'0010' */
#define CMD_TBLRD           0x8  /* 0b'1000' */
#define CMD_TBLRD_POST_INC  0x9  /* 0b'1001' */
#define CMD_TBLRD_POST_DEC  0xA  /* 0b'1010' */
#define CMD_TBLRD_PRE_INC   0xB  /* 0b'1011' */
#define CMD_TBLWR           0xC  /* 0b'1100' */
#define CMD_TBLWR_POST_INC2 0xD  /* 0b'1101' */
#define CMD_TBLWR_PROG_INC2 0xE  /* 0b'1110' */
#define CMD_TBLWR_PROG      0xF  /* 0b'1111' */

#define PIC_OFFSET_CODE     0x000000  /* Code space start */
#define PIC_END_CODE        0x100000  /* Code space end */
#define PIC_OFFSET_ID       0x200000  /* ID space */
#define PIC_OFFSET_CONFIG   0x300000  /* Config data register space */
#define PIC_END_CONFIG      0x300010  /* Config data register space end */

#define WRITEBUF_SIZE 64

#define P9  (1000*1)  /* 1ms */
#define P9A (1000*5)  /* 5ms */
#define P10 200       /* 200us */
#define P11 (1000*5)  /* 4ms */
#define P12 2         /* 2us */
#define P15 2         /* 2us */
#define P16 0
#define P18 0

#define BCHUNK_MAX 256

typedef struct {
    const unsigned char *cur_ptr;
    const unsigned char *data;
    unsigned int length;
    unsigned int offset;
} bchunk_t;

void bchunk_init(bchunk_t *chunk)
{
    const unsigned char *data = pic_data;

    data += 4;

    memset(chunk, 0, sizeof(*chunk));
    chunk->cur_ptr = data;
}

int bchunk_next(bchunk_t *chunk)
{
    const unsigned char *data = chunk->cur_ptr;
    int offset;

    switch (data[1]) {
        case '0':
            offset = data[3];
            break;
        case '1':
            offset = (data[3]<<8) + data[4];
            break;
        case '2':
            offset = (data[3]<<16) + (data[4]<<8) + data[5];
            break;
        case '3':
            offset = (data[3]<<24) + (data[4]<<16) + (data[5]<<8) + data[6];
            break;
        case '8':
            return -1;
        default:
            PIC_NOTICE("ERROR: PIC data - illegal type: %02x\n", data[1]);
            return -1;
    }

    const unsigned char *dptr = data + 4 + (data[1] - 0x30);
    int dlen = data[2] - 2 - (data[1] - 0x30);
    data += (3 + data[2]);
    
    chunk->cur_ptr = data;
    chunk->data    = dptr;
    chunk->offset  = offset;
    chunk->length  = dlen;

    PIC_TRACE("Chunk: 0x%04x, len %d\n", chunk->offset, chunk->length);

    return chunk->length;
}

static __attribute__ ((unused)) const char *cmd2bin(u8 cmd) 
{
    switch (cmd) {
        case  0: return "0000";
        case  1: return "0001";
        case  2: return "0010";
        case  3: return "0011";
        case  4: return "0100";
        case  5: return "0101";
        case  6: return "0110";
        case  7: return "0111";
        case  8: return "1000";
        case  9: return "1001";
        case 10: return "1010";
        case 11: return "1011";
        case 12: return "1100";
        case 13: return "1101";
        case 14: return "1110";
        case 15: return "1111";
        default:;
    }
    return "???";
}

static void pic_wait(int wait)
{
    if(wait) {
        DELAY_US(wait);
    }
}

static void pic_gpio_oe(int gpio, BOOL oe)
{
    vcoreiii_gpio_set_input(gpio, !oe);
}

static void pic_gpio_set(int gpio, int val)
{
    vcoreiii_gpio_set_output_level(gpio, val);
}

static int pic_gpio_read(int gpio)
{
    u32 st = vcoreiii_io_readl(&VTSS_DEVCPU_GCB_GPIO_GPIO_IN);
    return (st & VTSS_BIT(gpio)) ? 1 : 0;
}

static void pic_set_pgm(int val)
{
    pic_gpio_set(GPIO_PGM, !val);  /* Inverted */
}

static void pic_set_mclr(int val)
{
    pic_gpio_set(GPIO_MCLR, val);
}

static void pic_set_pgc(int val)
{
    pic_gpio_set(GPIO_PGC, !val);  /* Inverted */
}

static void pic_set_pgd(int val)
{
    pic_gpio_set(GPIO_PGD, val);
}

static void pic_set_pins(int pgm_val, int mclr_val, int pgc_val, int pgd_val)
{
    pic_set_mclr(mclr_val);
    pic_set_pgc(pgc_val);
    pic_set_pgd(pgd_val);
    pic_set_pgm(pgm_val);
}

static void pic_gpio_init(void)
{
    /* Protect against reset - PHY0/GPIO0 */
    (void) vtss_phy_write(NULL, 0, 16 | VTSS_PHY_REG_GPIO, 0x0000); /* Level = 0 */
    (void) vtss_phy_write(NULL, 0, 17 | VTSS_PHY_REG_GPIO, 0x0001); /* OE - GPIO0 */

    pic_set_pins(0, 0, 0, 0);
    pic_gpio_oe(GPIO_PGM,  TRUE); /* PGM */
    pic_gpio_oe(GPIO_MCLR, TRUE); /* MCLR */
    pic_gpio_oe(GPIO_PGC,  TRUE); /* PGC */
    pic_gpio_oe(GPIO_PGD,  TRUE); /* PGD (I/O) */
}

static void pic_gpio_deinit(BOOL reboot)
{
    if (!reboot) {
        pic_set_pgd(0);
        pic_gpio_oe(GPIO_PGM,  FALSE); /* PGM */
        pic_gpio_oe(GPIO_MCLR, FALSE); /* MCLR */
        pic_gpio_oe(GPIO_PGC,  FALSE); /* PGC */
        pic_wait(500*1000);
        (void) vtss_phy_write(NULL, 0, 17 | VTSS_PHY_REG_GPIO, 0x0000); /* nOE - GPIO0 */
        PIC_TRACE("PIC operation done, NOT rebooting\n");
    } else {
        PIC_NOTICE("Programming done, rebooting system\n");
        (void) vtss_phy_write(NULL, 0, 17 | VTSS_PHY_REG_GPIO, 0x0000); /* nOE - GPIO0 */
        pic_gpio_oe(GPIO_PGD,  FALSE); /* PGD */
        pic_gpio_oe(GPIO_PGM,  FALSE); /* PGM */
        pic_gpio_oe(GPIO_MCLR, FALSE); /* MCLR */
        pic_gpio_oe(GPIO_PGC,  FALSE); /* PGC */
        while(1) {
            /* Wait for reset */
        }
    }
}

static void pic_ll_enter(void)
{
    pic_set_pins(0, 0, 0, 0);
    pic_set_pgm(1);
    pic_wait(P15);
    pic_set_mclr(1);
    pic_wait(P12);
}

static void pic_ll_exit(void)
{
    pic_set_pgd(0);
    pic_wait(P16);
    pic_set_mclr(0);
    pic_wait(P18);
    pic_set_pins(0, 0, 0, 0);
}

static void pic_pulse(u8 byte, int nbits, u8 *read)
{
    int i;
    u8 input = 0;
    if(read) {
        pic_gpio_oe(GPIO_PGD, FALSE);
    }
    pic_set_pgc(0);
    for (i = 0; i < nbits; i++, byte >>= 1) {
        if (!read) {
            pic_set_pgd(byte & 1);
        }
        pic_set_pgc(1);
        pic_set_pgc(0);
        if(read) {
            if (pic_gpio_read(GPIO_PGD)) {
                input |= (1 << i);
            }
        }
    }
    if(read) {
        pic_gpio_oe(GPIO_PGD, TRUE);
        *read = input;
    }
}

static void pic_write_cmd(u8 cmd)
{
    pic_pulse(cmd, 4, NULL);
}

static void pic_write_byte(u8 byte)
{
    pic_pulse(byte, 8, NULL);
}

static void pic_read_byte(u8 byte, u8 *read)
{
    pic_pulse(byte, 8, read);
}

static void _pic_cmd(u8 cmd, u8 b0, u8 b1, int pause)
{
    PIC_TRACE("-- %s %02x %02x\n", cmd2bin(cmd), b0, b1);
    pic_write_cmd(cmd);
    if (pause) {
        pic_wait(pause);
    }
    pic_write_byte(b1);  /* NB - reverse order! */
    pic_write_byte(b0);
}

static void _pic_core(u8 b0, u8 b1, int pause)
{
    _pic_cmd(CMD_CORE, b0, b1, pause);
}

static void pic_core(u8 b0, u8 b1)
{
    _pic_core(b0, b1, 0);
}

static void pic_xcmd(u8 cmd, u8 b0, u8 b1)
{
    _pic_cmd(cmd, b0, b1, 0);
}

#define MOVLW   0x0E
#define MOVWF   0x6E
#define TBLPTRU 0xF8
#define TBLPTRH 0xF7
#define TBLPTRL 0xF6
#define NOP     0x00

static void pic_wr_en_code()
{
    pic_core(0x8E, 0xA6);  /* BSF EECON1, EEPGD */
    pic_core(0x9C, 0xA6);  /* BCF EECON1, CFGS */
    pic_core(0x84, 0xA6);  /* BSF EECON1, WREN */
}

static void pic_wr_en_config()
{
    pic_core(0x8E, 0xA6);  /* BSF EECON1, EEPGD */
    pic_core(0x8C, 0xA6);  /* BSF EECON1, CFGS */
    pic_core(0x84, 0xA6);  /* BSF EECON1, WREN */
}

static void pic_wr_dis(void)
{
    pic_core(0x94, 0xA6);  /* BCF EECON1, WREN */
}

static void pic_set_offset(int offset)
{
    pic_core(MOVLW, (offset >> 16) & 0xFF);
    pic_core(MOVWF, TBLPTRU);
    pic_core(MOVLW, (offset >>  8) & 0xFF);
    pic_core(MOVWF, TBLPTRH);
    pic_core(MOVLW, (offset >>  0) & 0xFF);
    pic_core(MOVWF, TBLPTRL);
}

static u8 pic_table_read(void)
{
    u8 data;
    pic_write_cmd(CMD_TBLRD_POST_INC);
    pic_write_byte(0);
    pic_read_byte(0, &data);
    return data;
}

static void pic_read(u8 *buf, size_t len, int offset)
{
    int i;
    pic_set_offset(offset);
    for (i = 0; i < len; i++) {
        buf[i] = pic_table_read();
    }
}

static void pic_erase(u8 b0, u8 b1)
{
    pic_set_offset(0x3c0005);
    pic_xcmd(CMD_TBLWR, b0, b0);
    pic_set_offset(0x3c0004);
    pic_xcmd(CMD_TBLWR, b1, b1);
    pic_core(NOP, NOP);
    _pic_core(NOP, NOP, P11);
}

static void pic_program_nop(int clk_wait_high, int clk_wait_low)
{
    int i;
    PIC_TRACE("-- %s %02x %02x - PROG (High: %d us, Low: %d us)\n", cmd2bin(0), 0, 0, clk_wait_high, clk_wait_low);
    for (i = 0; i < 3; i++) {
        pic_set_pgc(1);
        pic_set_pgc(0);
    }
    pic_set_pgc(1);
    pic_wait(clk_wait_high);
    pic_set_pgc(0);
    pic_wait(clk_wait_low);
    for (i = 0; i < 16; i++) {
        pic_set_pgc(1);
        pic_set_pgc(0);
    }
}

static void pic_program_row(const u8 *data, int len)
{
    int i;
    for (i = 0; i < (len-2); i += 2) {
        pic_xcmd(CMD_TBLWR_POST_INC2, data[i+1], data[i]);
    }
    pic_xcmd(CMD_TBLWR_PROG_INC2, data[i+1], data[i]);
    pic_program_nop(P9, P10);
}

mesa_rc pic_program_code(void)
{
    bchunk_t chk;

    pic_wr_en_code();
    pic_erase(0x0F, 0x8F);  /* Chip Erase */
    
    bchunk_init(&chk);
    while(bchunk_next(&chk) > 0) {
        if (chk.offset >= PIC_OFFSET_CODE && chk.offset < PIC_END_CODE) {
            int i, left;
            if ((chk.offset % WRITEBUF_SIZE) == 0) {
                pic_set_offset(chk.offset);
                for (i = 0, left = chk.length; left > 0; i += WRITEBUF_SIZE, left -= WRITEBUF_SIZE) {
                    pic_program_row(chk.data + i, left > WRITEBUF_SIZE ? WRITEBUF_SIZE : left);
                }
            } else {
                u8 data[BCHUNK_MAX];
                int al_off = chk.offset & ~(WRITEBUF_SIZE-1);
                int al_gap = chk.offset - al_off;
#if defined(ALL_RECORDS_UNALIGNED)
                pic_read(data, al_off, al_gap);
#else
                memset(data, 0xFF, al_gap);  /* We assume blank bytes - always */
#endif
                pic_set_offset(al_off);
                memcpy(data + al_gap, chk.data, chk.length);
                for (i = 0, left = chk.length + al_gap; left > 0; i += WRITEBUF_SIZE, left -= WRITEBUF_SIZE) {
                    pic_program_row(data + i, left > WRITEBUF_SIZE ? WRITEBUF_SIZE : left);
                }
            }
        }
    }
    pic_wr_dis();

    return VTSS_RC_OK;
}

unsigned char mask_data(int offset)
{
    /* Special masks for config data */
    if (offset >= PIC_OFFSET_CONFIG && offset < PIC_END_CONFIG) {
        return config_mask[offset - PIC_OFFSET_CONFIG];
    }
    /* Full mask for rest */
    return 0xFF;
}

mesa_rc pic_program_conf(BOOL erase)
{
    int i;
    bchunk_t chk;
    u8 current[BCHUNK_MAX];

    pic_wr_en_config();
    if (erase) {
        pic_erase(0x00, 0x82);  /* Config Bits Erase */
    }
    bchunk_init(&chk);
    while(bchunk_next(&chk) > 0) {
        if (chk.offset >= PIC_OFFSET_CONFIG && chk.offset < PIC_END_CONFIG) {
            pic_read(current, chk.length, chk.offset);
            for (i = 0; i < chk.length; i++) {
                u8 byte = chk.data[i];
                u8 mask = mask_data(chk.offset+i);
                if((mask & current[i]) != (mask & byte)) {
                    PIC_TRACE("Config[0x%x]: Have 0x%02x, need 0x%02x\n", chk.offset+i, mask & current[i], mask & byte);
                    pic_set_offset(chk.offset+i);
                    pic_xcmd(CMD_TBLWR_PROG, byte, byte);
                    pic_program_nop(P9A, P10);
                }
            }
        }
    }
    pic_wr_dis();

    return VTSS_RC_OK;
}

mesa_rc board_pic_read(u8 *data, int offset, int len)
{
    pic_gpio_init();

    pic_ll_enter();
    pic_read(data, len, offset);
    pic_ll_exit();

    pic_gpio_deinit(FALSE);

    return VTSS_RC_OK;
}

static mesa_rc pic_verify(void)
{
    bchunk_t chk;
    u8 data[BCHUNK_MAX];
    int i;

    bchunk_init(&chk);
    while(bchunk_next(&chk) > 0) {

        pic_ll_enter();
        pic_read(data, chk.length, chk.offset);
        pic_ll_exit();

        //diag_dump_buf_with_offset((u8 *)data,     chk.length, (u8 *)(data - chk.offset));
        //diag_dump_buf_with_offset((u8 *)chk.data, chk.length, (u8 *)(chk.data - chk.offset));

        for (i = 0; i < chk.length; i++) {
            unsigned char mask = mask_data(chk.offset+i);
            if ((mask & data[i]) != (mask & chk.data[i])) {
                PIC_TRACE("Compare error at offset 0x%04x - expected 0x%02x, got 0x%02x - mask %02x\n", 
                            chk.offset + i, mask & chk.data[i], mask & data[i], mask);
                return VTSS_UNSPECIFIED_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc board_pic_verify(void)
{
    mesa_rc rc;
    pic_gpio_init();
    rc = pic_verify();
    pic_gpio_deinit(FALSE);
    return rc;
}

mesa_rc board_pic_program(BOOL reboot)
{
    pic_gpio_init();

    pic_ll_enter();
    pic_program_code();
    pic_program_conf(FALSE);
    pic_ll_exit();

    pic_gpio_deinit(reboot);

    return VTSS_RC_OK;
}

mesa_rc board_pic_erase(void)
{
    pic_gpio_init();

    pic_ll_enter();
    pic_wr_en_code();
    pic_erase(0x0F, 0x8F);  /* Chip Erase */
    pic_wr_dis();
    pic_ll_exit();

    pic_gpio_deinit(FALSE);

    return VTSS_RC_OK;
}

void board_pic_check_update(void)
{
    pic_gpio_init();

    if (pic_verify() != VTSS_RC_OK) {
        /* LED state: Firmware update */
        led_front_led_state(LED_FRONT_LED_FLASHING_BOARD, TRUE);
        PIC_NOTICE("NOTICE: PIC code needs updating... DO NOT TURN POWER OFF! ...");
        pic_ll_enter();
        pic_program_code();
        pic_program_conf(FALSE);  /* Don't erase conf first - already erased */
        pic_ll_exit();
        pic_gpio_deinit(TRUE);
        /* NOT REACHED */
        PIC_NOTICE("PIC programming returned without reboot?\n");
    }

    pic_gpio_deinit(FALSE);
    PIC_NOTICE("No PIC update needed.\n");
}

#endif  /* VTSS_SW_OPTION_PIC */
