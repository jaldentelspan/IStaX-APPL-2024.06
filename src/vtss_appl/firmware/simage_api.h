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

#ifndef _SIMAGE_API_H_
#define _SIMAGE_API_H_

#define SIMAGE_SIG     "[(#)VtssImageTrailer(#)]"
#define SIMAGE_SIGLEN  24
#define SIMAGE_KEY     "THjwbSx!w(yShw71-ShS18%153&|91jshjdi" /* Arbitrarily chosen */
#define SIMAGE_TCOOKIE 0x64972FEB                             /* Before backpointer */

#define SIMAGE_DIGLEN        20 /* SHA1 digest len */

enum {
    SIMAGE_TLV_ARCH = 1,
    SIMAGE_TLV_CHIP,
    SIMAGE_TLV_IMGTYPE,
    SIMAGE_TLV_IMGSUBTYPE,

    /* Insert new TLV's above this line */
    SIMAGE_TLV_COUNT
};

enum {
    SIMAGE_TLV_TYPE_DWORD = 1,
    SIMAGE_TLV_TYPE_STRING,
    SIMAGE_TLV_TYPE_BINARY,
};

enum {
    SIMAGE_IMGTYPE_BOOT_LOADER = 1,
    SIMAGE_IMGTYPE_SWITCH_APP,
};

/* Individual TLV after parsing */
typedef struct {
    BOOL     valid;
    u32      type;
    u32      dw_value;
    const u8 *str_ptr;
    u32      str_len;
} simage_tlv_t;

typedef struct {
    /* Main Image data */
    const u8 *img_ptr;
    size_t    img_len;
    /* File data - ptr as above */
    size_t    file_len;
    u8        file_digest[SIMAGE_DIGLEN];
    /* Trailer data */
    const u8 *trailer_ptr;
    size_t    trailer_len;
    u8        trailer_digest[SIMAGE_DIGLEN];
    /* Parsed trailer */
    simage_tlv_t tlv[SIMAGE_TLV_COUNT];
} simage_t;

mesa_rc simage_parse(simage_t *simg, const u8 *image, size_t img_len);

BOOL simage_get_tlv_dword(const simage_t *simg, int id, u32 *val);

#endif // _SIMAGE_API_H_

