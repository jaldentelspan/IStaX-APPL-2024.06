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

#include <sys/ioctl.h>
#include <mbedtls/pk.h>      // public key module
#include <mbedtls/md.h>
#include <unistd.h>
#include <errno.h>

#include "firmware_vimage.h"

/*
 * --- New image support ---
 */

#define DER_KEY_SIZE 294

int mscc_vimage_hdrcheck(mscc_firmware_vimage_t *fw, const char **errmsg)
{
    if (fw->hdrlen < sizeof(*fw)) {
        if (errmsg) *errmsg = "Short header";
        return -1;
    }
    if (fw->magic1 != MSCC_VIMAGE_MAGIC1 ||
        fw->magic2 != MSCC_VIMAGE_MAGIC2 ||
        fw->version < 1) {
        if (errmsg) *errmsg = "Image format invalid";
        return -1;
    }
#if 0
    if (!vimage_socfamily_valid(fw->soc_no)) {
        if (errmsg) *errmsg = "Image with wrong chipset family";
        return -1;
    }
#endif
    return 0;    // All OK
}

static bool load_auth_key(const char *fis_key, mbedtls_pk_context *key)
{
    vtss_mtd_t mtd;
    bool loaded = false;
    mbedtls_pk_init(key);
    if (vtss_mtd_open(&mtd, fis_key) == VTSS_OK) {
        u8 *kdata = (u8 *) alloca(mtd.info.size);
        if (!kdata) {
            T_E("firmware authentication: Memory allocation of %d bytes failed", mtd.info.size);
        } else {
            int ret;
            if (read(mtd.fd, kdata, DER_KEY_SIZE) != DER_KEY_SIZE) {
                T_W("Read error on mtd: %s!", fis_key);
            } else {
                /* try importing the key - DER format */
                if ( (ret = mbedtls_pk_parse_public_key(key, kdata, DER_KEY_SIZE)) == 0) {
                    loaded = true;
                } else {
                    T_W("Firmware key parse failed!");
                }
            }
        }
        vtss_mtd_close(&mtd);
    } else {
        T_W("No firmware authentication key installed.");
    }
    return loaded;
}

static bool do_sha_validate(const void *data,
                            size_t data_len,
                            const char *sha_name,
                            const u8 *signature, u32 sig_size)
{
    mbedtls_pk_context fw_key;
    const mbedtls_md_info_t *mdinfo;
    u8 digest2[MBEDTLS_MD_MAX_SIZE];
    if (!load_auth_key("fwkey", &fw_key)) {
        return false;
    }
    mdinfo = mbedtls_md_info_from_string(sha_name);
    if (!mdinfo) {
        T_E("Hash type %s not available.", sha_name);
        return false;
    }
    if (mbedtls_md(mdinfo, (const unsigned char*)data, data_len, digest2) == 0) {
        int ret;
        if ((ret = mbedtls_pk_verify(&fw_key, mbedtls_md_get_type(mdinfo),
                                     digest2, mbedtls_md_get_size(mdinfo),
                                     signature, sig_size)) == 0) {
            T_I("Firmware was rsa-%s authenticated", sha_name);
            return true;
        } else {
            T_W("Firmware rsa-%s authentication error.", sha_name);
        }
    } else {
        T_W("Hash function %s error.", sha_name);
    }
    return false;
}

static bool md5_calc(const void *data, size_t data_length, void *digest)
{
    const mbedtls_md_info_t *mdinfo;
    mdinfo = mbedtls_md_info_from_string("MD5");
    if (mdinfo) {
        if (mbedtls_md(mdinfo, (const unsigned char*)data, data_length, (unsigned char*)digest) == 0) {
            return true;
        } else {
            T_E("MD5 Hash error");
        }
    } else {
        T_E("MD5 Hash type not available.");
    }
    return false;
}

bool mscc_vimage_validate(mscc_firmware_vimage_t *fw, mscc_firmware_vimage_tlv_t *tlv, u8 *signature)
{
    u8 *sigcopy;

    // check signature sanity
    switch (fw->img_sign_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            if (secure_enforce || tlv->data_len != MD5_MAC_LEN)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            if (tlv->data_len < 256 || tlv->data_len > 1024)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            if (tlv->data_len < 256 || tlv->data_len > 1024)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_NULL:
        default:
        invalid_signature:
            T_D("Image has invalid signature");
            return false;
    }

    // Allocate signature copy while re-calculating
    sigcopy = (u8 *) alloca(tlv->data_len);
    if (!sigcopy) {
        return false;    // Failure
    }
    // Copy orig signature
    memcpy(sigcopy, signature, tlv->data_len);
    // Reset to zero
    memset(signature, 0, tlv->data_len);

    switch (fw->img_sign_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            {
                if (md5_calc(fw, fw->imglen, signature) &&
                    memcmp(sigcopy, signature, tlv->data_len) == 0) {
                    T_I("MD5 signature validated");
                    return true;
                }
            }
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            {
                if (do_sha_validate(fw, fw->imglen, "SHA256", sigcopy, tlv->data_len)) {
                    memcpy(signature, sigcopy, tlv->data_len);    // Restore signature
                    return true;
                }
            }
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            {
                if (do_sha_validate(fw, fw->imglen, "SHA512", sigcopy, tlv->data_len)) {
                    memcpy(signature, sigcopy, tlv->data_len);    // Restore signature
                    return true;
                }
            }
            break;
        default:
            T_W("Type %d signature is unsupported", fw->img_sign_type);
    }
    T_I("Failed to validate type %d signature", fw->img_sign_type);
    return false;    // Failed auth
}

u8 *mscc_vimage_find_tlv(const mscc_firmware_vimage_t *fw,
                         mscc_firmware_vimage_tlv_t *tlv,
                         mscc_firmware_image_stage1_tlv_t type)
{
    size_t off = sizeof(*fw);
    mscc_firmware_vimage_tlv_t *wtlv;

    while (off < fw->imglen) {
        wtlv = (mscc_firmware_vimage_tlv_t*) (((u8*)fw) + off);
        if (type == wtlv->type) {
            *tlv = *wtlv;
            return &wtlv->value[0];
        }
        off += wtlv->tlv_len;
    }

    return NULL;
}

off_t mscc_vimage_mtd_find_tlv(const mscc_firmware_vimage_t *fw,
                             vtss_mtd_t *mtd,
                             mscc_firmware_vimage_tlv_t *tlv,
                             mscc_firmware_image_stage1_tlv_t type)
{
    off_t off = sizeof(*fw);
    mscc_firmware_vimage_tlv_t wtlv;

    while (off < fw->imglen) {
        if (pread(mtd->fd, &wtlv, sizeof(wtlv), off) != sizeof(wtlv))
            break;
        if (type == wtlv.type) {
            *tlv = wtlv;
            return off + sizeof(wtlv);
        }
        off += wtlv.tlv_len;
    }

    return 0;
}


size_t mscc_firmware_stage1_get_imglen_fd(int fd)
{
    mscc_firmware_vimage_t fw;     // MFI style
    const char *errmsg = "short read";
    size_t len = 0;
    if (pread(fd, &fw, sizeof(fw), 0) == sizeof(fw) &&
        mscc_vimage_hdrcheck(&fw, &errmsg) == 0) {
        len = fw.imglen;
        T_I("MFI image found, len %zd", len);
    } else {
        T_I("No valid fw found");
    }
    return len;
}

size_t mscc_firmware_stage1_get_imglen(vtss_mtd_t *mtd)
{
    return mscc_firmware_stage1_get_imglen_fd(mtd->fd);
}

off_t mscc_firmware_sideband_get_offset(vtss_mtd_t *mtd)
{
    off_t base;
    size_t len = mscc_firmware_stage1_get_imglen(mtd);
    if (len) {
        base = MSCC_FIRMWARE_ALIGN(len, mtd->info.erasesize);
        T_I("MFI image found, len %08zx, sb @ %08lx", len, (unsigned long)base);
    } else {
        base = 0;
    }
    return base;
}

mscc_firmware_sideband_t *mscc_vimage_sideband_read(vtss_mtd_t *mtd, off_t offset)
{
    mscc_firmware_sideband_t hdr, *sb = NULL;
    const char *errmsg = "short read";
    if (pread(mtd->fd, &hdr, sizeof(hdr), offset) == sizeof(hdr) &&
        mscc_vimage_sideband_check(&hdr, &errmsg) == 0) {
        sb = (mscc_firmware_sideband_t *) VTSS_MALLOC(hdr.length);
        if (sb) {
            T_I("Read sideband data at offset %08lx", (unsigned long)offset);
            (void) pread(mtd->fd, sb, hdr.length, offset);
        }
    } else {
        T_D("Sideband read: %s", errmsg);
    }
    // Now check CRC
    if (sb) {
        u8 signature[MD5_MAC_LEN], hash[MD5_MAC_LEN];
        memcpy(signature, sb->checksum, MD5_MAC_LEN); // Save signature
        memset(sb->checksum, 0, MD5_MAC_LEN);         // Null out for recalc
        // Recalc & Compare
        if (md5_calc(sb, sb->length, hash) &&
            memcmp(hash, signature, MD5_MAC_LEN) == 0) {
            T_I("Sideband MD5 signature validated");
        } else {
            T_I("Sideband MD5 signature invalid");
            VTSS_FREE(sb);
            sb = NULL;
        }
    }
    return sb;
}

vtss_rc mscc_vimage_sideband_write(const vtss_mtd_t *mtd,
                                   mscc_firmware_sideband_t *sb,
                                   const off_t base)
{
    vtss_rc rc = VTSS_OK;
    size_t offset, written;
    struct erase_info_user mtdEraseInfo;
    for (offset = 0; offset < sb->length; offset += mtd->info.erasesize) {
        mtdEraseInfo.start = base + offset;
        mtdEraseInfo.length = mtd->info.erasesize;
        T_D("Erase sideband at offset %08x", mtdEraseInfo.start);
        (void) ioctl(mtd->fd, MEMUNLOCK, &mtdEraseInfo);
        if (ioctl (mtd->fd, MEMERASE, &mtdEraseInfo) < 0) {
            T_E("%s: Erase error at 0x%08zx: %s", mtd->dev, (size_t) mtdEraseInfo.start, strerror(errno));
            rc = FIRMWARE_ERROR_FLASH_ERASE;
            break;
        }
    }
    if (rc == VTSS_OK) {
        T_I("Writing sideband at offset %08lx", (unsigned long)base);
        if ((written = pwrite(mtd->fd, sb, sb->length, base)) == sb->length) {
            T_I("Sideband wrote %zd bytes at %08lx", written, (unsigned long)base);
        } else {
            T_E("Sideband write: %zd vs. %d: %s", written, sb->length, strerror(errno));
            rc = FIRMWARE_ERROR_FLASH_PROGRAM;
        }
    }
    return rc;
}

u8 *mscc_vimage_sideband_find_tlv(const mscc_firmware_sideband_t *sb,
                                  mscc_firmware_vimage_tlv_t *tlv,
                                  mscc_firmware_image_sideband_tlv_t type)
{
    size_t off = sizeof(*sb);
    mscc_firmware_vimage_tlv_t *wtlv;
    while ((off + sizeof(mscc_firmware_vimage_tlv_t)) < sb->length) {
        wtlv = (mscc_firmware_vimage_tlv_t*) (((u8*)sb) + off);
        if (type == wtlv->type) {
            *tlv = *wtlv;
            return &wtlv->value[0];
        }
        off += wtlv->tlv_len;
    }
    return NULL;
}

int mscc_vimage_sideband_check(const mscc_firmware_sideband_t *sb, const char **errmsg)
{
    if (sb->length < sizeof(*sb)) {
        if (errmsg) *errmsg = "Short header";
        return -1;
    }
    if (sb->magic1 != MSCC_VIMAGE_SB_MAGIC1 ||
        sb->magic2 != MSCC_VIMAGE_SB_MAGIC2) {
        if (errmsg) *errmsg = "Sideband format invalid";
        return -1;
    }
    return 0;    // All OK
}

mscc_firmware_sideband_t *mscc_vimage_sideband_create(void)
{
    mscc_firmware_sideband_t *sb = (mscc_firmware_sideband_t *) VTSS_MALLOC(sizeof(*sb));
    if (sb) {
        sb->length = sizeof(*sb);
        sb->magic1 = MSCC_VIMAGE_SB_MAGIC1;
        sb->magic2 = MSCC_VIMAGE_SB_MAGIC2;
    }
    return sb;
}

mscc_firmware_sideband_t *mscc_vimage_sideband_add_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_len,
                                                       mscc_firmware_image_sideband_tlv_t type)
{
    size_t tlvlen = sizeof(mscc_firmware_vimage_tlv_t) + MSCC_FIRMWARE_TLV_ALIGN(data_len);
    T_D("SB %p, Size %d, adding %zd bytes, data %zd", sb, sb->length, tlvlen, data_len);
    sb = (mscc_firmware_sideband_t*) VTSS_REALLOC((void*)sb, sb->length + tlvlen);
    if (sb) {
        // Add the tlv at back
        mscc_firmware_vimage_tlv_t *tlv= (mscc_firmware_vimage_tlv_t *) (((u8 *)sb) + sb->length);
        tlv->type = type;
        tlv->tlv_len = tlvlen;
        tlv->data_len = data_len;
        memcpy(tlv->value, data, data_len);
        sb->length += tlvlen;
    }
    return sb;
}

mscc_firmware_sideband_t *mscc_vimage_sideband_set_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_len,
                                                       mscc_firmware_image_sideband_tlv_t type)
{
    mscc_firmware_vimage_tlv_t tlv;
    const char *name;
    if ((name = (const char *) mscc_vimage_sideband_find_tlv(sb, &tlv, MSCC_FIRMWARE_SIDEBAND_FILENAME))) {
        mscc_vimage_sideband_delete_tlv(sb, type);
    }
    return mscc_vimage_sideband_add_tlv(sb, data, data_len, type);
}

void mscc_vimage_sideband_delete_tlv(mscc_firmware_sideband_t *sb,
                                     mscc_firmware_image_sideband_tlv_t type)
{
    size_t off = sizeof(*sb);
    while ((off + sizeof(mscc_firmware_vimage_tlv_t)) < sb->length) {
        mscc_firmware_vimage_tlv_t *wtlv = (mscc_firmware_vimage_tlv_t*) (((u8*)sb) + off);
        if (type == wtlv->type) {
            u32 tlvlen = wtlv->tlv_len;
            // Move up data
            memcpy((u8*) wtlv, ((u8*) wtlv) + tlvlen, (sb->length - off) - tlvlen);
            // Adjust len
            sb->length -= tlvlen;
        } else {
            off += wtlv->tlv_len;
        }
    }
}

void mscc_vimage_sideband_update_crc(mscc_firmware_sideband_t *sb)
{
    memset(sb->checksum, 0, MD5_MAC_LEN);
    (void) md5_calc(sb, sb->length, sb->checksum);
}

// Stage2

static bool check_stage2_sig(const mscc_firmware_vimage_stage2_tlv_t *tlv,
                             const mscc_firmware_vimage_stage2_tlv_t *tlv_data)
{
    const char *hash = NULL;
    const u8 *signature = &tlv_data->value[0] + tlv->data_len;
    u32 siglen = tlv->tlv_len - tlv->data_len - sizeof(*tlv);

    switch (tlv->sig_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            if (secure_enforce) {
                goto illegal_signature;
            }
            if (siglen == MD5_MAC_LEN) {
                u8 calcsig[MD5_MAC_LEN];
                if (md5_calc(tlv_data, tlv->tlv_len - siglen, calcsig) &&
                    memcmp(calcsig, signature, sizeof(calcsig)) == 0) {
                    T_I("MD5 signature validated");
                    return true;
                }
                T_W("MD5 signature invalid");
            } else {
                T_W("Illegal MD5 hash length: %d", siglen);
            }
            return false;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            hash = "SHA256";
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            hash = "SHA512";
            break;
        default:
        illegal_signature:
            T_W("Illegal signature type: %d", tlv->sig_type);
            return false;
    }

    // These are sane ranges for SHA signatures
    if (siglen < 256 || siglen > 1024) {
        T_W("Illegal SHA length: %d", siglen);
        return false;
    }

    // Now validate
    T_D("Validate data %p length %d, signature at %p length %d", tlv_data, tlv->tlv_len, signature, siglen);
    return do_sha_validate(tlv_data, tlv->tlv_len - siglen, hash, signature, siglen);
}

bool mscc_vimage_stage2_check_tlv(const mscc_firmware_vimage_stage2_tlv_t *s2tlv,
                                  size_t s2len)
{
    mscc_firmware_vimage_stage2_tlv_t tlvcopy;

    memcpy(&tlvcopy, s2tlv, sizeof(tlvcopy));

    // Size check, TLV
    if (tlvcopy.tlv_len > s2len) {
        T_D("TLV len %d exceeds buffer length %zd", tlvcopy.tlv_len, s2len);
        return false;
    }

    // Magic checks
    if (tlvcopy.magic1 != MSCC_VIMAGE_S2TLV_MAGIC1) {
        T_D("TLV magic error %08x", tlvcopy.magic1);
        return false;
    }

    // Size check, data
    if (tlvcopy.data_len > (tlvcopy.tlv_len - sizeof(tlvcopy))) {
        T_D("TLV data len %d exceeds tlv length %d, hdr %zd", tlvcopy.data_len, tlvcopy.tlv_len, sizeof(tlvcopy));
        return false;
    }

    // Type
    if (tlvcopy.type == 0 || tlvcopy.type >= MSCC_STAGE2_TLV_LAST) {
        T_D("TLV type %d invalid", tlvcopy.type);
        return false;
    }

    // Validate signature
    if (check_stage2_sig(&tlvcopy, s2tlv) != true) {
        T_D("TLV signature validation failed");
        return false;
    }

    T_D("TLV length %d/%d OK", tlvcopy.tlv_len,  tlvcopy.data_len);
    return true;
}
