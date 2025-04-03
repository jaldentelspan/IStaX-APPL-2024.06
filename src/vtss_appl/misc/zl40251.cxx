/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/i2c-dev.h>  /* I2C support */
#include <errno.h>

#undef  VTSS_ALLOC_MODULE_ID
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MISC

#include "main.h"
#include "misc.h"
#include "icli_porting_util.h"
#include "misc_icli_util.h"


int zl40251_pps = 0b1110100;
int zl40251_ptp_clock = 0b1110101;

bool fa_zl40251_read_verify(int i2c_addr, unsigned char *pattern, int pattern_size)
{
    char *filename = "/dev/i2c-0";
    int file;

    if ((file = open(filename, O_RDWR)) < 0) {
        T_D("open(%s) failed: %s", filename, strerror(errno));
        return false;
    }

    if (ioctl(file, I2C_SLAVE, i2c_addr) < 0) {
        T_E("Cannot specify i2c slave at 0x%02x (file = %s) [%s]", i2c_addr, filename, strerror(errno));
        close(file);
        return false;
    }

    char set_read[] = {0x03, 0x00, 0x00};
    int cnt = write(file, set_read, sizeof(set_read));
    if (cnt != sizeof(set_read)) {
        T_E("Failed writing %zd bytes to %s address %02x. Wrote %d bytes. Error: %s", sizeof(set_read), filename, i2c_addr, cnt, strerror(errno));
    } else {
        T_D("Wrote %d bytes to %s address %0x\n", cnt, filename, i2c_addr);
    }

    unsigned char chip[pattern_size];
    cnt = read(file, chip, pattern_size);
    if (cnt != pattern_size) {
        T_E("Failed control reading from i2c address: %x. Read %d bytes, expected %d bytes",
            i2c_addr, cnt, pattern_size);
    }

    close(file);
    return memcmp(pattern, chip, pattern_size) == 0;
}

void fa_zl40251_init(int i2c_addr, unsigned char *eeprom, int eeprom_size)
{
    char *filename = "/dev/i2c-0";
    int file;

    if ((file = open(filename, O_RDWR)) < 0) {
        T_D("open(%s) failed: %s", filename, strerror(errno));
        return;
    }

    if (ioctl(file, I2C_SLAVE, i2c_addr) < 0) {
        T_E("Cannot specify i2c slave at 0x%02x (file = %s) [%s]", i2c_addr, filename, strerror(errno));
        close(file);
        return;
    }

    // Configure the eeprom for access from I2C
    unsigned char map_eeprom[] = {0x02, 0x00, 0x00, 0x80};
    int cnt = write(file, map_eeprom, sizeof(map_eeprom));
    if (cnt != sizeof(map_eeprom)) {
        T_D("Failed writing %zd bytes to %s address %02x. Wrote %d bytes. Error: %s\n", sizeof(map_eeprom), filename, i2c_addr, cnt, strerror(errno));
        close(file);
        return;
    } else {
        T_D("Wrote %d bytes to %s address %0x\n", cnt, filename, i2c_addr);
    }

    // Enable write mode for
    char write_mode[] = {0x06};
    int i=0;
    while (i < eeprom_size) {
        cnt = write(file, write_mode, sizeof(write_mode));
        if (cnt != sizeof(write_mode)) {
            T_E("Failed configuring block %d for write", i/32);
        }

        int j;
        char datablock[35];
        datablock[0] = 0x02;
        datablock[1] = (i&0xFF00)>>8;
        datablock[2] = i&0xFF;
        for (j = 0; j<32 && i+j < eeprom_size; ++j) {
            datablock[j+3] = eeprom[i+j];
        }
        int blocksize = j+3;
        cnt = write(file, datablock, blocksize);
        if (cnt != blocksize) {
            T_E("Failed writing datablock %d size %d", i/32, blocksize);
        }

        i += j;
        VTSS_MSLEEP(5);

    }

    char set_read[] = {0x03, 0x00, 0x00};
    cnt = write(file, set_read, sizeof(set_read));
    if (cnt != sizeof(set_read)) {
        T_E("Failed writing %zd bytes to %s address %02x. Wrote %d bytes. Error: %s", sizeof(set_read), filename, i2c_addr, cnt, strerror(errno));
    } else {
        T_D("Wrote %d bytes to %s address %0x\n", cnt, filename, i2c_addr);
    }

    unsigned char buf[eeprom_size];
    char hexdump[10000];
    char *p_hexdump = hexdump;
    cnt = read(file, buf, eeprom_size);
    if (cnt != eeprom_size || memcmp(buf, eeprom, eeprom_size) != 0) {
        if (cnt != eeprom_size) {
            T_E("Failed control reading from i2c address: %x. Read %d bytes, expected %d bytes",
                i2c_addr, cnt, eeprom_size);
        } else {
            for (int k=0; k<cnt; ++k) {
                p_hexdump += sprintf(p_hexdump, "%02x ", buf[k]);
                if ((k+1)%16 == 0) {
                    p_hexdump += sprintf(p_hexdump, "\n");
                } else if ((k+1)%8 == 0) {
                    p_hexdump += sprintf(p_hexdump, " ");
                }
            }
            T_E("Failed control reading from i2c address: %x. Read:\n%s", i2c_addr, hexdump);
        }
    }

    // Unmap eeprom for access from I2C
    unsigned char unmap_eeprom[] = {0x02, 0x00, 0x00, 0x00};
    cnt = write(file, unmap_eeprom, sizeof(unmap_eeprom));
    if (cnt != sizeof(unmap_eeprom)) {
        T_E("Failed writing %zd bytes to %s address %02x. Wrote %d bytes. Error: %s\n", sizeof(unmap_eeprom), filename, i2c_addr, cnt, strerror(errno));
    } else {
        T_D("Wrote %d bytes to %s address %0x\n", cnt, filename, i2c_addr);
    }

    close(file);
}

static const char *eeprom_filename = "/etc/mscc/misc/ZL40251_EEPROM_Image.txt";
static unsigned char eeprom[3000];

static int readfile()
{
    int fd;
    struct stat st = {0};
    char *zl_conf_data;

    if ((fd = open(eeprom_filename, O_RDONLY)) < 0) {
        T_E("Could not open %s for reading.\n", eeprom_filename );
        return -1;
    }
    if (fstat(fd, &st) < 0) {
        close(fd);
        T_E("Could not determine size of %s.\n", eeprom_filename );
        return -1;
    }
    if ((zl_conf_data = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
        close(fd);
        T_E("Could not map %s.\n", eeprom_filename );
        return -1;
    }

    int buf_index = 0;
    bool skipping = false;
    ;
    for (char *p_zl_conf_data = zl_conf_data;
         p_zl_conf_data < zl_conf_data + st.st_size && *p_zl_conf_data != 0; p_zl_conf_data++) {
        if (*p_zl_conf_data == '\n') {
            skipping = false;
            continue;
        }
        if (skipping) {
            continue;
        }
        if (*p_zl_conf_data == ';') {
            T_N("Skipping line");
            skipping = true;
            continue;
        }

        unsigned int value;
        int match = sscanf(p_zl_conf_data, "%2x", &value);
        if (match != 1) {
            printf("No value\n");
            continue;
        }
        eeprom[buf_index++] = value;
        skipping = true; // wait for end of line before starting to look for next value
    }
    (void) munmap(zl_conf_data, st.st_size);
    close(fd);
    return buf_index;
}


mesa_rc zl40251_initialization()
{
    unsigned char expected[16] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0xad, 0x01, 0x15,
                                   0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    int eeprom_size = readfile();
    
    if (!fa_zl40251_read_verify(zl40251_pps, expected, 16)) {
        printf("Programming zl40251 at I2C address 0x%x\n", zl40251_pps);
        fa_zl40251_init(zl40251_pps, eeprom, eeprom_size);
    }
    if (!fa_zl40251_read_verify(zl40251_ptp_clock, expected, 16)) {
        printf("Programming zl40251 at I2C address 0x%x\n", zl40251_ptp_clock);
        fa_zl40251_init(zl40251_ptp_clock, eeprom, eeprom_size);
    }

    return VTSS_RC_OK;
}
