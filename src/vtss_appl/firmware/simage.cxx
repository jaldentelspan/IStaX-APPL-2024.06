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

#include "main.h"
#include "firmware.h"
#include "firmware_api.h"

typedef struct {
    const u8 *buf_ptr;
    size_t    buf_len;
} bufptr_t;

static const u8 *
bufsearch(const u8 *haystack, size_t length, const u8 *needle, size_t nlength)
{
    const u8 *end = haystack+length;
    while(haystack < end &&
          (haystack = (const u8*)memchr(haystack, needle[0], end - haystack))) {
        if(nlength <= (size_t)(end - haystack) &&
           memcmp(haystack, needle, nlength) == 0)
            return haystack;
        haystack++;
    }
    return NULL;
}

static u32 get_dword(const u8 *ptr)
{
    return (u32) ((ptr[0]) + 
                  (ptr[1] << 8) +
                  (ptr[2] << 16) +
                  (ptr[3] << 24));
}

static inline void buf_init(bufptr_t *b, const u8 *ptr, size_t len)
{
    b->buf_ptr = ptr;
    b->buf_len = len;
}

static inline void buf_skip(bufptr_t *b, size_t n)
{
    if(n <= b->buf_len) {
        b->buf_ptr += n;
        b->buf_len -= n;
    } else {
        b->buf_ptr += b->buf_len;
        b->buf_len = 0;
    }
}

static u32 get_dword_advance(bufptr_t *b)
{
    u32 val = get_dword(b->buf_ptr);
    buf_skip(b, sizeof(u32));
    return val;
}

static void simage_init(simage_t *simg, const u8 *image, size_t img_len)
{
    memset(simg, 0, sizeof(*simg));
    simg->img_ptr = image;
    simg->img_len = img_len;
}

static BOOL simage_validate_hash(simage_t *simg, const u8 *p)
{
    const u8 *ptrs = p;
    ptrs += SIMAGE_SIGLEN;      /* Skip Signature */
    simg->file_len = get_dword(ptrs); ptrs += 4;
    memcpy(simg->file_digest, ptrs, SIMAGE_DIGLEN); ptrs += SIMAGE_DIGLEN;
    simg->trailer_len = get_dword(ptrs); ptrs += 4;
    memcpy(simg->trailer_digest, ptrs, SIMAGE_DIGLEN); ptrs += SIMAGE_DIGLEN;
    simg->trailer_ptr = ptrs;
    if(simg->file_len < simg->img_len &&
       (simg->trailer_ptr + simg->trailer_len) < (simg->img_ptr + simg->img_len)) {
        T_D("Image validated, now parsing");
        return TRUE;
    }
    T_D("Ptr consistency error: lengths (file %zu, img %zu), img %p, trailer(%p , %zu)",
        simg->file_len, simg->img_len, simg->img_ptr, simg->trailer_ptr, simg->trailer_len);
    return FALSE;
}

static const u8 *simage_search_trailer(simage_t *simg, bufptr_t *b)
{
    const u8 *hit;
    if(b->buf_len &&
       (hit = bufsearch(b->buf_ptr, b->buf_len, (const u8*)SIMAGE_SIG, SIMAGE_SIGLEN))) {
        buf_skip(b, hit - b->buf_ptr); /* Skip down */
        T_D("Found signature at %p, buffer left %zu", hit, b->buf_len);
        return b->buf_ptr;
    }
    T_D("No signature match");
    return NULL;
}

static mesa_rc simage_get_trailer(simage_t *simg, bufptr_t *b)
{
    u32 backp;
    const u8 *p;
    /* Try to use the backpointer, if found */
    p = (b->buf_ptr + b->buf_len); /* End of buffer */
    if(get_dword(p - 2*sizeof(u32)) == SIMAGE_TCOOKIE &&
       (backp = get_dword(p - 1*sizeof(u32))) < b->buf_len &&
       memcmp(p - backp, SIMAGE_SIG, SIMAGE_SIGLEN) == 0) {
        T_D("Signature found with backpointer: %d", backp);
        p -= backp;
        return simage_validate_hash(simg, p) ? VTSS_RC_OK : (mesa_rc)FIRMWARE_ERROR_CRC;
    } else {
        /* Must search from start */
        while((p = simage_search_trailer(simg, b))) {
            if(simage_validate_hash(simg, p))
                return VTSS_RC_OK; 
            else 
                /* Try for other matches */
                buf_skip(b, SIMAGE_SIGLEN);
        }
    }

    /* All failed */
    return FIRMWARE_ERROR_INVALID;
}

mesa_rc simage_parse_tlvs(simage_t *simg)
{
    bufptr_t buf;
    buf_init(&buf, simg->trailer_ptr, simg->trailer_len);
    while(buf.buf_len > 3*sizeof(u32)) {
        u32 type, id, dlen;
        type = get_dword_advance(&buf);
        id = get_dword_advance(&buf);
        dlen = get_dword_advance(&buf);
        if(id > 0 && id < SIMAGE_TLV_COUNT) {
            simage_tlv_t *tlv = &simg->tlv[id];
            T_N("type %d, id %d dlen %d", type, id, dlen);
            switch(type) {
            case SIMAGE_TLV_TYPE_DWORD:
                tlv->valid = TRUE;
                tlv->type = type;
                tlv->dw_value = get_dword(buf.buf_ptr);
                T_D("Dword id %d value %d", id, tlv->dw_value);
                break;
            case SIMAGE_TLV_TYPE_STRING:
                tlv->valid = TRUE;
                tlv->type = type;
                tlv->str_len = dlen;
                tlv->str_ptr = buf.buf_ptr;
                T_D("Str id %d value %s", id, buf.buf_ptr);
                break;
            case SIMAGE_TLV_TYPE_BINARY:
                tlv->valid = TRUE;
                tlv->type = type;
                tlv->str_len = dlen;
                tlv->str_ptr = buf.buf_ptr;
                T_D("Bin id %d len %u", id, dlen);
                break;
            default:
                T_I("Skip invalid type: type %d, id %d dlen %d", type, id, dlen);
            }
        } else {
            T_I("Skip invalid id: %d, id %d dlen %d", type, id, dlen);
        }
        buf_skip(&buf, dlen);
    }
    /* Always succeeds, even if skipped TLV's */
    return VTSS_RC_OK;
}

mesa_rc simage_parse(simage_t *simg, const u8 *image, size_t img_len)
{
    bufptr_t buf;
    mesa_rc rc;
    simage_init(simg, image, img_len);
    buf_init(&buf, image, img_len);
    if((rc = simage_get_trailer(simg, &buf)) != VTSS_RC_OK)
        return rc; 
    return simage_parse_tlvs(simg);
}

BOOL simage_get_tlv_dword(const simage_t *simg, int id, u32 *val)
{
    if(id > 0 && id < SIMAGE_TLV_COUNT) {
        const simage_tlv_t *tlv = &simg->tlv[id];
        if(tlv->valid && tlv->type == SIMAGE_TLV_TYPE_DWORD) {
            *val = tlv->dw_value;
            return TRUE;
        }
    } 
    return FALSE;
}

