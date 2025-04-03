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

#ifndef __FIRMWARE_UBI_HXX__
#define __FIRMWARE_UBI_HXX__

#include <mtd/ubi-user.h>
#include "vtss_mtd_api.hxx"

class Firmware_ubi {
 public:
  // constructor
  Firmware_ubi();
  // constructor
  Firmware_ubi(const char *mtd_name, int ubi_num_, int vol_id_, int hdr_offset_);
  // umount filesystem
  mesa_rc ubiumount();
  // detaches MTD devices from UBI devices
  mesa_rc ubidetach();
  // formats empty flash, erases flash and preserves erase counters, flashes UBI
  // images to MTD devices.
  mesa_rc ubiformat();
  // attaches MTD devices
  mesa_rc ubiattach();
  // create volume with specific size limit. If mbytes_limit = 0 then all available space is used.
  mesa_rc ubimkvol(unsigned int mbytes_limit, const char *vol_name = nullptr);
  // mount filesystem
  mesa_rc ubimount(const char *vol_name = "switch", const char *mnt_point = "/switch");
  mesa_rc ubiupdatevol(const unsigned char *img, int size);
  // determine avail space
  long long available_space();
  mesa_rc get_ubi_mtd_num(char *mtd_num, int mtd_num_sz);

  // the mtd holding "rootfs_data" / "rootfs_nand_data"
  vtss_mtd_t data_mtd;
  // the ubi device number corresponding to mtd_num;
  int ubi_num;
  // volume number
  int vol_id;
    // header offset, set to 0 to use default
  int hdr_offset;
};

#endif  // __FIRMWARE_UBI_HXX__
