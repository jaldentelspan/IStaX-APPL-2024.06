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

#ifndef _SYNCE_SPI_IF_H_
#define _SYNCE_SPI_IF_H_

#include "main_types.h"

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define VTSS_SPI_CS_NONE 0xFF

struct synce_clock_chip_spi_if {
    bool si5326 = false;
    bool zarlink = false;

    void write(u8 address, u8 data);
    void write_masked(u8 address, u8 data, u8 mask);
    void zl_3034x_write(u32 address, u8 *data, u32 size);
    void read(u8 address, u8 *data);
    void zl_3034x_read(u32 address, u8 *data, u32 size);
    void zl_3034x_load_file(const char *filename);
};

struct synce_t1e1j1_spi_if {
    int dev;
    char dev_name[17];
    struct spi_ioc_transfer xfer_dev;

    int init(const char *dev_filename);
    void spi_transfer(u32 buflen, const u8 *tx_data, u8 *rx_data);

    void write(u8 address, u8 data);
    void write_masked(u8 address, u8 data, u8 mask);
    void read(u8 address, u8 *data);
};

struct synce_cpld_spi_if {
    int dev;
    char dev_name[17];
    struct spi_ioc_transfer xfer_dev;

    int init(const char *dev_filename);
    void spi_transfer(u32 buflen, const u8 *tx_data, u8 *rx_data);

    void write(u8 address, u8 data);
    void read(u8 address, u8 *data);
};

namespace vtss
{
namespace synce
{
namespace dpll
{
extern synce_clock_chip_spi_if clock_chip_spi_if;
} // namespace dpll
} // namespace synce
} // namespace vtss

#endif // _SYNCE_SPI_IF_H_
