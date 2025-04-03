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

#include <sys/ioctl.h>
#include <mbedtls/pk.h>      // public key module
#include <mbedtls/md.h>

#include "main.h"
#include "firmware.h"
#include "firmware_vimage.h"
#include "vtss_mtd_api.hxx"

struct md_frag {
    const uint8_t *ptr;
    size_t        len;
};

int vimage_socfamily_valid(int socfamily)
{
    int running = fast_cap(MESA_CAP_SOC_FAMILY);

    T_D("Checking F/W image family (%d) against currently running image's family (%d)", socfamily, running);
    return socfamily == running;
}

/*
 * --- New image support ---
 */

int mscc_vimage_hdrcheck(const mscc_firmware_vimage_t *fw, const char **errmsg)
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
    if (!vimage_socfamily_valid(fw->soc_no)) {
        if (errmsg) *errmsg = "Image with wrong chipset family";
        return -1;
    }
    return 0;    // All OK
}

static bool load_auth_key(const char *fis_key, mbedtls_pk_context *key)
{
    vtss_mtd_t mtd;
    bool loaded = false;
    mbedtls_pk_init(key);
    if (vtss_mtd_open(&mtd, fis_key) == VTSS_RC_OK) {
        u8 *kdata = (u8 *) alloca(mtd.info.size);
        if (!kdata) {
            T_E("firmware authentication: Memory allocation of %d bytes failed", mtd.info.size);
        } else {
            int ret;
            if (read(mtd.fd, kdata, mtd.info.size) != mtd.info.size) {
                T_W("Read erroe on mtd: %s!", fis_key);
            } else {
                /* try importing the key - DER format */
                if ( (ret = mbedtls_pk_parse_public_key(key, kdata, mtd.info.size)) == 0) {
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

static int calc_md_n(const mbedtls_md_info_t *mdinfo,
                     const struct md_frag *frags,
                     size_t nfrags,
                     int hmac,
                     unsigned char *output)
{
    mbedtls_md_context_t ctx;
    int ret, i;
    mbedtls_md_init(&ctx);
    if ((ret = mbedtls_md_setup(&ctx, mdinfo, hmac)) == 0 &&
        (ret = mbedtls_md_starts(&ctx)) == 0) {
        for (i = 0; i < nfrags; i++) {
            if ((ret = mbedtls_md_update(&ctx, frags[i].ptr, frags[i].len)) != 0) {
                goto out;
            }
        }
        ret = mbedtls_md_finish(&ctx, output);
  out:
        mbedtls_md_free(&ctx);
    }
    return ret;
}

static bool do_sha_validate_n(const struct md_frag *frags,
                              size_t nfrags,
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
    if (calc_md_n(mdinfo, frags, nfrags, 1, digest2) == 0) {
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

static bool md5_calc_n(const struct md_frag *frags,
                       size_t nfrags,
                       u8 *digest)
{
    const mbedtls_md_info_t *mdinfo;
    mdinfo = mbedtls_md_info_from_string("MD5");
    if (mdinfo) {
        if (calc_md_n(mdinfo, frags, nfrags, 0, digest) == 0) {
            return true;
        } else {
            T_E("MD5 Hash error");
        }
    } else {
        T_E("MD5 Hash type not available.");
    }
    return false;
}

static bool do_sha_validate(const void *data,
                            size_t data_len,
                            const char *sha_name,
                            const u8 *signature, u32 sig_size)
{
    struct md_frag hashdata;
    hashdata.len = data_len;
    hashdata.ptr = (const uint8_t *) data;
    return do_sha_validate_n(&hashdata, 1, sha_name, signature, sig_size);
}

static bool md5_calc(const void *data, size_t data_len, u8 *digest)
{
    struct md_frag hashdata;
    hashdata.len = data_len;
    hashdata.ptr = (const uint8_t *) data;
    return md5_calc_n(&hashdata, 1, digest);
}

bool mscc_vimage_validate(const mscc_firmware_vimage_t *fw, const u8 *signature, uint32_t siglen)
{
    u8 *calc_sig, *headptr;
    struct md_frag hashdata[3];    // Head, signature(zeroes), tail;

    // check signature sanity
    switch (fw->img_sign_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            if (siglen != MD5_MAC_LEN)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            if (siglen < 256 || siglen > 1024)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            if (siglen < 256 || siglen > 1024)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_NULL:
        default:
        invalid_signature:
            T_D("Image has invalid signature");
            return false;
    }

    // Allocate zero signature
    siglen = siglen;
    calc_sig = (u8 *) alloca(siglen);
    if (!calc_sig) {
        return false;    // Failure
    }
    // Reset to zero before calculating
    memset(calc_sig, 0, siglen);

    // Check that signature is *embedded* in image
    headptr = (u8 *)fw;
    if (signature > headptr && signature < (headptr + fw->imglen)) {
        hashdata[0].ptr = headptr;
        hashdata[0].len = signature - headptr;    // Length of initial chunk
        hashdata[1].ptr = calc_sig;
        hashdata[1].len = siglen;
        hashdata[2].ptr = signature + siglen;
        hashdata[2].len = fw->imglen -  hashdata[0].len -  hashdata[1].len;
    } else {
        T_D("Signature lies outside image");
        return false;
    }

    switch (fw->img_sign_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            {
                if (md5_calc_n(hashdata, ARRSZ(hashdata), calc_sig) &&
                    memcmp(calc_sig, signature, siglen) == 0) {
                    T_I("MD5 signature validated");
                    return true;
                }
            }
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            {
                if (do_sha_validate_n(hashdata, ARRSZ(hashdata), "SHA256", signature, siglen)) {
                    return true;
                }
            }
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            {
                if (do_sha_validate_n(hashdata, ARRSZ(hashdata), "SHA512", signature, siglen)) {
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

u8 *mscc_vimage_mtd_read_tlv(const char *mtd_name,
                             mscc_firmware_vimage_t *fw,
                             mscc_firmware_vimage_tlv_t *tlv,
                             mscc_firmware_image_stage1_tlv_t type)
{
    vtss_mtd_t mtd;
    mesa_rc rc;
    u8 *tlvbuf = NULL;
    const char *msg;
    off_t toff;

    if ((rc = vtss_mtd_open(&mtd, mtd_name)) != VTSS_RC_OK) {
        T_D("Failed to open mtd: %s", mtd_name);
        return NULL;
    }

    /* Read the stage1 header of that image */
    if (pread(mtd.fd, fw, sizeof(*fw), 0) != sizeof(*fw)) {
        T_D("Failed to read header of existing image");
        goto EXIT;
    }
    if (mscc_vimage_hdrcheck(fw, &msg)) {
        T_D("Invalid header: %s", msg);
        goto EXIT;
    }

    if ((toff = mscc_vimage_mtd_find_tlv(fw, &mtd, tlv, type)) &&
        (tlvbuf = (u8*) VTSS_MALLOC(tlv->data_len + 1)) != NULL &&
        pread(mtd.fd, tlvbuf, tlv->data_len, toff) == tlv->data_len) {
        tlvbuf[tlv->data_len] = '\0';
        T_D("Got TLV %d, length %d", type, tlv->data_len);
    } else {
        T_D("No TLV %d", type);
        if (tlvbuf) {
            VTSS_FREE(tlvbuf);
            tlvbuf = NULL;
        }
    }

EXIT:
    vtss_mtd_close(&mtd);
    return tlvbuf;
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

// Getting vimage information.
mesa_rc mscc_firmware_fw_vimage_get(vtss_mtd_t *mtd, mscc_firmware_vimage_t *fw) {
    const char *errmsg = "short read";
    if (pread(mtd->fd, fw, sizeof(*fw), 0) == sizeof(*fw) &&
        mscc_vimage_hdrcheck(fw, &errmsg) == 0) {
        T_I("MFI image found, len %d", fw->imglen);
        T_I("magic1:0x%X, magic2:0x%X", fw->magic1, fw->magic2);
        T_I("version:%d, hdrlen:%d", fw->version, fw->hdrlen);
        T_I("Machine:%s, Soc::%s", fw->machine, fw->soc_name);
        return VTSS_RC_OK;
    }

    T_D("No valid fw found");
    return FIRMWARE_ERROR_IMAGE_NOT_FOUND;
}

off_t mscc_firmware_sideband_get_offset(vtss_mtd_t *mtd)
{
    mscc_firmware_vimage_t fw;     // MFI style
    const char *errmsg = "short read";
    off_t base = 0;
    if (pread(mtd->fd, &fw, sizeof(fw), 0) == sizeof(fw) &&
        mscc_vimage_hdrcheck(&fw, &errmsg) == 0) {
        base = MSCC_FIRMWARE_ALIGN(fw.imglen, mtd->info.erasesize);
        T_D("MFI image found, len %zd, sb @ %08llx", fw.imglen, base);
        T_D("magic1:0x%X, magic2:0x%X", fw.magic1, fw.magic2);
        T_D("version:%d, hdrlen:%d", fw.version, fw.hdrlen);
        T_D("Machine:%s, Soc::%s", fw.machine, fw.soc_name);
    } else {
        T_I("No valid fw found");
    }
    T_D("Base:%llu",base);
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
            T_I("Read sideband data at offset %08llx", offset);
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

mesa_rc mscc_vimage_sideband_write(const vtss_mtd_t *mtd,
                                   mscc_firmware_sideband_t *sb,
                                   const off_t base)
{
    mesa_rc rc = VTSS_RC_OK;
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
    if (rc == VTSS_RC_OK) {
        T_I("Writing sideband at offset %08llx", base);
        if ((written = pwrite(mtd->fd, sb, sb->length, base)) == sb->length) {
            T_I("Sideband wrote %zd bytes at %08llx", written, base);
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
#if !defined(MSCC_OPT_FIRMWARE_ENFORCE_PKI)
            if (siglen == MD5_MAC_LEN) {
                u8 calcsig[MD5_MAC_LEN];
                if (md5_calc(tlv_data, tlv->tlv_len - siglen, calcsig) &&
                    memcmp(calcsig, signature, sizeof(calcsig)) == 0) {
                    T_D("MD5 signature validated");
                    return true;
                }
                T_W("MD5 signature invalid");
            } else {
                T_W("Illegal MD5 hash length: %d", siglen);
            }
            return false;
#endif
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            hash = "SHA256";
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            hash = "SHA512";
            break;
        default:
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
                                  size_t s2len, bool check_sig)
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
    if (check_sig && check_stage2_sig(&tlvcopy, s2tlv) != true) {
        T_D("TLV signature validation failed");
        return false;
    }

    T_D("TLV length %d/%d OK", tlvcopy.tlv_len,  tlvcopy.data_len);
    return true;
}

mscc_firmware_vimage_stage2_tlv_t *mscc_vimage_stage2_filename_tlv_create(const char* filename, uint32_t *tlvlen)
{
    if (filename == nullptr || strlen(filename) == 0) {
        return nullptr;
    }

    uint32_t namelen = strlen(filename) + 1;
    *tlvlen = sizeof(mscc_firmware_vimage_stage2_tlv_t) + namelen + MD5_MAC_LEN;

    mscc_firmware_vimage_stage2_tlv_t *s2tlv = (mscc_firmware_vimage_stage2_tlv_t *) VTSS_MALLOC(*tlvlen);
    if (s2tlv == nullptr) {
        T_D("Unable to allocate %d bytes for TLV", *tlvlen);
        *tlvlen = 0;
        return nullptr;
    }

    s2tlv->magic1 = MSCC_VIMAGE_S2TLV_MAGIC1;
    s2tlv->tlv_len = *tlvlen;
    s2tlv->data_len = namelen;
    s2tlv->type = MSCC_STAGE2_TLV_FILENAME;
    s2tlv->sig_type = MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5;
    strcpy((char *)s2tlv->value, filename);

    u8 *signature = &s2tlv->value[0] + namelen;
    if (!md5_calc(s2tlv, s2tlv->tlv_len - MD5_MAC_LEN, signature)) {
        VTSS_FREE(s2tlv);
        s2tlv = nullptr;
    }

    return s2tlv;
}

void mscc_vimage_stage2_filename_tlv_delete(mscc_firmware_vimage_stage2_tlv_t *s2tlv)
{
    VTSS_FREE(s2tlv);
}
