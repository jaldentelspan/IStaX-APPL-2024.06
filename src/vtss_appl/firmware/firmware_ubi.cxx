/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <mtd/ubi-user.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <errno.h>
#include <dirent.h>

#include "firmware_ubi.hxx"
#include "firmware.h"
#include <libubi.h>

#define DEVICE_UBI_CTRL "/dev/ubi_ctrl"
#define MOUNTS_INFO     "/proc/mounts"
//#define MOUNT_DIR       "/switch"
#define VOLUME_NAME     "switch"
#define UBI_NUM         -1
#define UBI_MAX_VOLUME_NAME 127

Firmware_ubi::Firmware_ubi() : vol_id(UBI_VOL_NUM_AUTO), hdr_offset(0)
{
  if (vtss_mtd_open(&data_mtd, "rootfs_data")      == VTSS_RC_OK ||
      vtss_mtd_open(&data_mtd, "rootfs_nand_data") == VTSS_RC_OK) {
    T_D("MTD '%s' found as [%s]", data_mtd.name, data_mtd.dev);
    vtss_mtd_close(&data_mtd);	// Don't need the fd
    for (int i=0; i<5; ++i) {
        char ubi_mtd_num[256];
        ubi_num = i;
        if (VTSS_RC_OK == get_ubi_mtd_num(ubi_mtd_num, sizeof(ubi_mtd_num))) {
            if ( strlen(ubi_mtd_num) == 1 && ubi_mtd_num[0] == data_mtd.devno+'0' ) {
                T_D("MTD '%s' bound to ubi%d_0", data_mtd.name, i);
                return;
            }
        }
    }
  } else {
    T_D("Neither rootfs_data nor rootfs_nand_data");
  }
  ubi_num = UBI_NUM;
}

Firmware_ubi::Firmware_ubi(const char *mtd_name, int ubi_num_, int vol_id_, int hdr_offset_)
        : ubi_num(ubi_num_), vol_id(vol_id_), hdr_offset(hdr_offset_)
{
  if (vtss_mtd_open(&data_mtd, mtd_name) == VTSS_RC_OK) {
    T_D("MTD '%s' found as [%s]", data_mtd.name, data_mtd.dev);
    vtss_mtd_close(&data_mtd);	// Don't need the fd
  } else {
    T_D("Did not find: %s", mtd_name);
  }
}

mesa_rc Firmware_ubi::ubiumount()
{
  mesa_rc rc = VTSS_RC_OK;
  FILE *fp;
  char buf[128];
  BOOL need_umount = FALSE;
  char *token;
  const char *delim = " ";
  char ubidev[10];

  T_D("device %s, ubi %d", data_mtd.name, ubi_num);
  fp = fopen(MOUNTS_INFO, "r");
  if (fp == NULL) {
    T_D("fopen %s failed [%s]", MOUNTS_INFO, strerror(errno));
    return VTSS_RC_ERROR;
  }

  sprintf(ubidev, "ubi%d", ubi_num);
  while ((fgets(buf, sizeof(buf), fp)) != NULL) {
    T_D("%s", buf);  // uncomment this for debug
    // process each line
    if (strstr(buf, "ubifs") && strstr(buf,ubidev)) {
      T_D("ubifs is present, need umount");
      need_umount = TRUE;
      break;
    }
  }

  if(need_umount) {
    // split targeted line
    char *saveptr; // Local strtok_r() context
    token = strtok_r(buf, delim, &saveptr);
    if (token != NULL) {
      token = strtok_r(NULL, delim, &saveptr);  // get mount point
    }
    // NOTE: invoke umount() from sys/mount.h
    if (::umount(token) == -1) {
      T_D("ubifs at %s umount failed [%s]", token, strerror(errno));
      rc = VTSS_RC_ERROR;
    }
    T_D("umount %s ok.", token);
  } else {
    T_D("No need to umount");
  }

  if (fclose(fp) == EOF) {
    T_D("Close %s failed [%s]", MOUNTS_INFO, strerror(errno));
    rc = VTSS_RC_ERROR;
  }
  return rc;
}

mesa_rc Firmware_ubi::ubidetach()
{
  mesa_rc rc = VTSS_RC_OK;
  int res;
  int fd;
  char str[16];  // ubi0
  struct stat stat_buf;
  BOOL need_detach = FALSE;

  // check if ubi_dev is attached
  sprintf(str, "/dev/ubi%d", ubi_num);
  if (stat(str, &stat_buf) == -1) {
    T_D("Expect %s not exist [%s]", str, strerror(errno));
  } else {
    need_detach = TRUE;
  }

  if (need_detach) {
    T_D("Need to detach ubi device [%s]", str);

    fd = open(DEVICE_UBI_CTRL, O_RDONLY);
    if (fd == -1) {
      T_D("Open mtd device %s failed [(%d): %s]", DEVICE_UBI_CTRL, fd, strerror(errno));
      return VTSS_RC_ERROR;
    }

    res = ioctl(fd, UBI_IOCDET, &ubi_num);
    close(fd);
    if (res == -1) {
      T_D("Detach ubi device (%s:%d) failed [%s]", DEVICE_UBI_CTRL, fd, strerror(errno));
      rc = VTSS_RC_ERROR;
      goto EXIT;
    }
    T_D("ubidetach [%s] ok", str);

  } else {
    T_D("No need to detach ubi device [%s]", str);
  }

EXIT:
  return rc;
}

mesa_rc Firmware_ubi::ubiformat()
{
    mesa_rc rc = VTSS_RC_OK;
    char cmd[64];  // ubiformat /dev/mtdX -y

    if (data_mtd.devno < 0) {
        T_D("Data mtd not found, not formatting.");
        rc = VTSS_RC_ERROR;
    } else {
        // ubiformat is rather complicated, just execute the shell command
        // ubiattach will catch the error, if ubiformat failed
        if (hdr_offset != 0) {
            sprintf(cmd, "ubiformat -O %d /dev/mtd%d -y", hdr_offset, data_mtd.devno);
        } else {
            sprintf(cmd, "ubiformat /dev/mtd%d -y", data_mtd.devno);
        }
        T_D("Execute: %s", cmd);
        if (system(cmd) == -1) {
            T_D("ubiformat /dev/mtd%d failed", data_mtd.devno);
            rc = VTSS_RC_ERROR;
        }
    }
    return rc;
}

mesa_rc Firmware_ubi::ubiattach()
{
  mesa_rc rc = VTSS_RC_OK;
  struct ubi_attach_req req;
  int fd, res;

  memset(&req, 0x0, sizeof(ubi_attach_req));

  if (data_mtd.devno < 0) {
    T_D("Invalid mtd number [%d] of rootfs_data", data_mtd.devno);
    return VTSS_RC_ERROR;
  }

  // check ubi-user.h for structure info
  req.ubi_num = ubi_num;
  req.mtd_num = data_mtd.devno;
  req.vid_hdr_offset = hdr_offset;
  req.max_beb_per1024 = 0;

  fd = open(DEVICE_UBI_CTRL, O_RDONLY);
  if (fd == -1) {
    T_D("Open %s failed [%s]", DEVICE_UBI_CTRL, strerror(errno));
    return VTSS_RC_ERROR;
  }

  if (ubi_num >= 0) {
      T_D("Execute: ubiatach -p /dev/mtd%d -d %d -O %d", data_mtd.devno, ubi_num, hdr_offset);
  } else {
      T_D("Execute: ubiatach -p /dev/mtd%d -O %d", data_mtd.devno, hdr_offset);
  }
  res = ioctl(fd, UBI_IOCATT, &req);
  if (res == -1) {
      T_D("Attach %s failed: %s", DEVICE_UBI_CTRL, strerror( errno ));
      rc = VTSS_RC_ERROR;
  } else {
      T_D("ubiattach ubi%d (mtd%d) ok.", req.ubi_num, req.mtd_num);
      if (ubi_num != req.ubi_num) {
          ubi_num = req.ubi_num;
      }
  }
  close(fd);
  return rc;

}

static unsigned long ubi_get_int_prop(int ubi_num, const char *node)
{
    unsigned long value = 0;
    vtss::StringStream ss;
    FILE *fp;
    print_fmt(ss, "/sys/class/ubi/ubi%d/%s", ubi_num, node);
    if ((fp = fopen(ss.cstring(), "r"))) {
        if (fscanf(fp, "%lu", &value) == 0) {
            value = 0;
        }
        fclose(fp);
    }
    T_D("%s = %lu", ss.cstring(), value);
    return value;
}

long long Firmware_ubi::available_space()
{
    long long avail_erase_blocks = ubi_get_int_prop(ubi_num, "avail_eraseblocks");
    long long eraseblock_size = ubi_get_int_prop(ubi_num, "eraseblock_size");
    return avail_erase_blocks * eraseblock_size;
}

mesa_rc Firmware_ubi::get_ubi_mtd_num(char *mtd_num, int mtd_num_sz)
{
    char fname[256];
    FILE *fp;
    mesa_rc rc = VTSS_RC_ERROR;
    if (mtd_num_sz<20) return rc;

    sprintf(fname, "/sys/class/ubi/ubi%d/mtd_num", ubi_num);
    if ((fp = fopen(fname, "r"))) {
        if (fscanf(fp, "%20s", mtd_num) > 0) {
            rc = VTSS_RC_OK;
        }
        fclose(fp);
    }
    return rc;
}

mesa_rc Firmware_ubi::ubimkvol(unsigned int mbytes_limit, const char *vol_name)
{
  mesa_rc rc = VTSS_RC_OK;
  int fd, res;
  struct ubi_mkvol_req req;
  size_t n;
  char str[64];

  memset(&req, 0x0, sizeof(struct ubi_mkvol_req));

  req.vol_id = vol_id;
  req.alignment = 1;  // default
  req.vol_type = UBI_DYNAMIC_VOLUME;
  if (mbytes_limit) {
      req.bytes = mbytes_limit * 1024UL * 1024UL;  // 64MiB
  } else {
      auto avail = available_space();
      req.bytes = avail;
  }


  strncpy(req.name, vol_name ? vol_name : VOLUME_NAME, UBI_MAX_VOLUME_NAME + 1);
  n = strlen(req.name);
  req.name_len = n;

  T_D("ubimkvol /dev/ubi%d -N %s -s %llu ", ubi_num, req.name, (unsigned long long) req.bytes);

  sprintf(str, "/dev/ubi%d", ubi_num);
  fd = open(str, O_RDONLY);
  if (fd == -1) {
    T_D("Open %s failed [%s]", str, strerror(errno));
    return VTSS_RC_ERROR;
  }

  res = ioctl(fd, UBI_IOCMKVOL, &req);

  close(fd);

  if (res == -1) {
    T_D("ubimkvol %s failed.", str);
    return VTSS_RC_ERROR;
  }

  vol_id = req.vol_id;
  T_D("ubimkvol %s ok.", str);
  return rc;
}

mesa_rc Firmware_ubi::ubiupdatevol(const unsigned char *img, int size)
{
  libubi_t ubi = libubi_open();
  char ubidev[64];
  sprintf(ubidev,"/dev/ubi%d_%d", ubi_num, vol_id);
  struct ubi_vol_info info;
  int ubi_rc = ubi_get_vol_info(ubi, ubidev, &info);
  if (ubi_rc) {
      T_D("ubi_get_vol_info: for %s failed [%s]", ubidev, strerror(errno));
      libubi_close(ubi);
      return VTSS_RC_ERROR;
  }
  int fd = open(ubidev, O_RDWR);
  if (fd == -1) {
      T_D("Open %s failed [%s]", ubidev, strerror(errno));
      libubi_close(ubi);
      return VTSS_RC_ERROR;
  }

  ubi_rc = ubi_update_start(ubi, fd, size);
  if (ubi_rc) {
      T_D("Update_start of %s failed [%s]", ubidev, strerror(errno));
      close(fd);
      libubi_close(ubi);
      return VTSS_RC_ERROR;
  }

  while (size) {
      auto written = write(fd, img, size);
      if (written < 0) {
          if (errno == EINTR) {
              T_D("EINTR!");
              continue;
          }
          close(fd);
          libubi_close(ubi);
          return VTSS_RC_ERROR;
      }

      if (written == 0) {
          close(fd);
          libubi_close(ubi);
          return VTSS_RC_ERROR;
      }

      size -= written;
      img += written;
  }

  close(fd);
  libubi_close(ubi);
  return VTSS_RC_OK;
}

mesa_rc Firmware_ubi::ubimount(const char *vol_name, const char *mnt_point)
{
  mesa_rc rc = VTSS_RC_OK;
  char str[16];

  sprintf(str, "ubi%d:%s", ubi_num, vol_name);
  T_D("Mount %s on %s", str, mnt_point);
  if (::mount(str, mnt_point, "ubifs", 0, NULL) == -1) {
    T_D("mount ubifs %s at %s failed [%s]", str, mnt_point, strerror(errno));
    return VTSS_RC_ERROR;
  }

  T_D("mount ubifs %s at %s ok.", str, mnt_point);
  return rc;
}
