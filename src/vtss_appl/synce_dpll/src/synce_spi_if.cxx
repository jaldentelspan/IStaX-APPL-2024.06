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

#include "main.h"
#include <stdio.h>
#include "synce_spi_if.h"
#include "backtrace.hxx"
#include <sys/mman.h>
#include <sys/stat.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYNCE_DPLL

#define SPI_SET_ADDRESS      0x00
#define SPI_WRITE            0x40
#define SPI_READ             0x80

// --- Implementation of member functions ---
void synce_clock_chip_spi_if::write(u8 address, u8 data)
{
    T_I("address %x, data %x", address, data);
    if (si5326) {
        (void) meba_synce_write(board_instance, SPI_SET_ADDRESS, 1, &address);
        (void) meba_synce_write(board_instance, SPI_WRITE, 1, &data);
    }

    if (zarlink) {
        address &= 0x7F;    /* Clear first bit to indicate write */
        (void) meba_synce_write(board_instance, address, 1, &data);
    }
}

static uint8_t page = 0;

void synce_clock_chip_spi_if::zl_3034x_write(u32 address, u8 *data, u32 size)
{
    char printBuf[128] = "";
    char *pPrintBuf = printBuf;
    if (size > 20) {
        return;
    }

    for (int i = 0; i < size && pPrintBuf - printBuf < sizeof(printBuf); ++i) {
        pPrintBuf += sprintf(pPrintBuf, "%s%02x", i == 0 ? "" : ":", data[i]);
    }

    if (address > 0x7F) {
        // ZL30731 is expecting page to be calculate in driver
        uint32_t new_page = address >> 7;
        address = address & 0x7F;
        if (new_page != page) {
            page = new_page;
            (void) meba_synce_write(board_instance, 0x7F, 1, &page);
            T_I("SPI write: address = %04x page = %d", address, page);
        }
    }

    if (address == 0x7f) {
        if (size != 1) {
            T_E("Wrong size of page field: %d (expected 1)\n", size);
        }
        page = data[0];
        T_D("SPI write: address = %04x page = %d", address, page);
    } else {
        T_I("SPI write: page: %d address = %04x data = %s", page, page * 128 + address, printBuf);
    }

    (void) meba_synce_write(board_instance, address, size, data);
}

void synce_clock_chip_spi_if::write_masked(u8 address, u8 data, u8 mask)
{
    u8 value;

    T_I("address %x, data %x, mask %x", address, data, mask);
    if (si5326) {
        meba_synce_read(board_instance, address, 1, &value);
        value = (value & ~mask) | (data & mask);
        meba_synce_write(board_instance, address, 1, &value);
    }

    if (zarlink) {
        meba_synce_read(board_instance, address | 0x80, 1, &value); /* Set first bit to indicate read */
        value = (value & ~mask) | (data & mask);
        meba_synce_write(board_instance, address & 0x7F, 1, &value); /* Clear first bit to indicate write */
    }
}

void synce_clock_chip_spi_if::read(u8 address, u8 *data)
{
    *data = 0;

    if (si5326) {
        (void) meba_synce_write(board_instance, SPI_SET_ADDRESS, 1, &address);
        (void) meba_synce_read(board_instance, SPI_READ, 1, data);
    }

    if (zarlink) {
        address |= 0x80;    /* Set first bit to indicate read */
        (void) meba_synce_read(board_instance, address, 1, data);
    }
    T_I("address %x, data %x", address, *data);
}

void synce_clock_chip_spi_if::zl_3034x_read(u32 address, u8 *data, u32 size)
{
    if (size > 20) {
        return;
    }

    if (address > 0x7F) {
        // ZL30731 is expecting page to be calculate in driver
        uint32_t new_page = address >> 7;
        address = address & 0x7F;
        if (new_page != page) {
            page = new_page;
            (void) meba_synce_write(board_instance, 0x7F, 1, &page);
        }
    }

    (void) meba_synce_read(board_instance, address, size, data);

    char printBuf[128] = "";
    char *pPrintBuf = printBuf;
    for (int i = 0; i < size && pPrintBuf - printBuf < sizeof(printBuf); ++i) {
        pPrintBuf += sprintf(pPrintBuf, "%s%02x", i == 0 ? "" : ":", data[i]);
    }

    if (address == 0x7f) {
        if (size != 1) {
            T_E("Wrong size of page field: %d (expected 1)\n", size);
        }
        T_D("SPI read: page = %s", printBuf);
    } else {
        T_I("SPI read: page = %d, address = %04x data = %s", page, page * 128 + address, printBuf);
    }
}

void synce_clock_chip_spi_if::zl_3034x_load_file(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    struct stat st = {0};
    char *f;
    if (fd < 0) {
        T_E("Could not open %s for reading. %s\n", filename, strerror(errno) );
        return;
    }
    if (fstat(fd, &st) < 0) {
        T_E("Could not stat %s. %s\n", filename, strerror(errno)  );
        return;
    }
    f = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (f == MAP_FAILED) {
        T_E("Could not map %s for reading. %s\n", filename, strerror(errno) );
        (void)close(fd);
        return;
    }

    bool is_comment = false;
    for (char *c = f; c < f + st.st_size; ++c) {
        if (*c == ';') {
            is_comment = true;
        }
        if (*c == '\n') {
            is_comment = false;
        }
        if (!is_comment && c[0] == 'X' && c[1] == ' ' && c[2] == ',' && c[3] == ' ') {
            uint32_t addr;
            uint32_t value;
            if (2 != sscanf(c + 4, "%x , %x", &addr, &value) || value > 0xff) {
                T_E("Failed reading (addr,value) from %s", filename);
            }
            uint8_t byte_value = value & 0xff;
            zl_3034x_write(addr, &byte_value, 1);
            c += 4;
        }
        if (!is_comment && c[0] == 'W' && c[1] == ' ' && c[2] == ',' && c[3] == ' ') {
            uint32_t wait_delay;
            sscanf(c + 4, "%d", &wait_delay);
            usleep(wait_delay);
            c += 4;
        }

    }
    (void) munmap(f, st.st_size);
    (void)close(fd);

}

int synce_t1e1j1_spi_if::init(const char *dev_filename)
{
    xfer_dev.delay_usecs = 1;
    strncpy(dev_name, dev_filename, sizeof(dev_name));

    dev = open(dev_name, O_RDWR);
    if (dev < 0) {
        T_D("Could not open SPI device file %s", dev_name);
        return 0;
    }
    T_D("Opened SPI device file %s.", dev_name);
    (void)close(dev);

    return 1;
}

void synce_t1e1j1_spi_if::spi_transfer(u32 buflen, const u8 *tx_data, u8 *rx_data)
{
    dev = open(dev_name, O_RDWR);
    if (dev < 0) {
        T_E("Could not open SPI device file %s for controlling", dev_name);
        return;
    }
    xfer_dev.tx_buf = (u64) tx_data;
    xfer_dev.rx_buf = (u64) rx_data;
    xfer_dev.len = buflen;
    int err_code;
    int timeout_count = 0;
    while ((err_code = ioctl(dev, SPI_IOC_MESSAGE(1), &xfer_dev)) == -1) {
        if (errno == EBUSY) {
            VTSS_MSLEEP(10);
            if (++timeout_count % 100 == 0) {
                T_E("Has been waiting %d second(s) for access to SPI IF.", (timeout_count / 100));
            }
            if (timeout_count == 1000) {
                T_E("Timeout while waiting for access to SPI IF (giving up now after waiting 10 seconds).");
            }
        } else {
            T_E("Got unexpected error code (errno = %d) from SPI ioctl call.", errno);
            break;
        }
    }
    (void)close(dev);
}

void synce_t1e1j1_spi_if::write(u8 address, u8 data)
{
    u8 tx_data[3], rx_data[3];

    tx_data[0] = address >> 7;
    tx_data[1] = address << 1;
    tx_data[2] = data;
    this->spi_transfer(3, tx_data, rx_data);
}

void synce_t1e1j1_spi_if::write_masked(u8 address, u8 data, u8 mask)
{
    u8 tx_data[3], rx_data[3];

    tx_data[0] = (address >> 7) | 0x80; /* Read command */
    tx_data[1] = address << 1;
    tx_data[2] = 0;
    this->spi_transfer(3, tx_data, rx_data);

    tx_data[0] &= 0x7F;                 /* Write command */
    tx_data[2] = (rx_data[2] & ~mask) | (data & mask);
    this->spi_transfer(3, tx_data, rx_data);
}

void synce_t1e1j1_spi_if::read(u8 address, u8 *data)
{
    u8 tx_data[3], rx_data[3];

    tx_data[0] = (address >> 7) | 0x80;
    tx_data[1] = address << 1;
    tx_data[2] = 0;
    this->spi_transfer(3, tx_data, rx_data);
    *data = rx_data[2];
}

int synce_cpld_spi_if::init(const char *dev_filename)
{
    xfer_dev.delay_usecs = 1;
    strncpy(dev_name, dev_filename, sizeof(dev_name));

    dev = open(dev_name, O_RDWR);
    if (dev < 0) {
        T_D("Could not open SPI device file %s", dev_name);
        return 0;
    }
    T_D("Opened SPI device file %s.", dev_name);
    (void)close(dev);

    return 1;
}

void synce_cpld_spi_if::spi_transfer(u32 buflen, const u8 *tx_data, u8 *rx_data)
{
    dev = open(dev_name, O_RDWR);
    if (dev < 0) {
        T_E("Could not open SPI device file %s for controlling", dev_name);
        return;
    }
    xfer_dev.tx_buf = (u64) tx_data;
    xfer_dev.rx_buf = (u64) rx_data;
    xfer_dev.len = buflen;
    int err_code;
    int timeout_count = 0;
    while ((err_code = ioctl(dev, SPI_IOC_MESSAGE(1), &xfer_dev)) == -1) {
        if (errno == EBUSY) {
            VTSS_MSLEEP(10);
            if (++timeout_count % 100 == 0) {
                T_E("Has been waiting %d second(s) for access to SPI IF.", (timeout_count / 100));
            }
            if (timeout_count == 1000) {
                T_E("Timeout while waiting for access to SPI IF (giving up now after waiting 10 seconds).");
            }
        } else {
            T_E("Got unexpected error code (errno = %d) from SPI ioctl call.", errno);
            break;
        }
    }
    (void)close(dev);
}

void synce_cpld_spi_if::write(u8 address, u8 data)
{
    u8 tx_data[2], rx_data[2];

    tx_data[0] = (address & 0x7F) | 0x80;
    tx_data[1] = data;
    this->spi_transfer(2, tx_data, rx_data);
}

void synce_cpld_spi_if::read(u8 address, u8 *data)
{
    u8 tx_data[2], rx_data[2];

    tx_data[0] = (address & 0x7F);
    tx_data[1] = 0;
    this->spi_transfer(2, tx_data, rx_data);
    *data = rx_data[1];
}

namespace vtss
{
namespace synce
{
namespace dpll
{
synce_clock_chip_spi_if clock_chip_spi_if;
}
}
}
