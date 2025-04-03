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

#ifndef _FIRMWARE_INFO_API_H_
#define _FIRMWARE_INFO_API_H_

#include "vtss/appl/firmware.h"
#include "firmware.h"
#include <list>
#include <map>
#include <vector>
#include "vtss_os_wrapper_linux.hxx"

#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
#include "json_rpc_api.hxx"
#include <vtss/basics/json/stream-parser.hxx>
#include <vtss/basics/json/stream-parser-callback.hxx>
#endif
#include <vtss/basics/trace.hxx>

//*****************************************************************************************
//* Because our public header doesn't support c++ std library, the public header
//* types are defined below.
//******************************************************************************************

//*****************************************************************************************
//* Internal types
//******************************************************************************************

// Key for the map containing the MFI information
struct firmware_info_key_t {
    u32 attribute_id;
    u32 section_id;
    u32 image_id;

    BOOL operator<(const firmware_info_key_t & n) const {
        if (this->image_id < n.image_id) return TRUE;
        if (this->image_id > n.image_id) return FALSE;

        if (this->section_id < n.section_id) return TRUE;
        if (this->section_id > n.section_id) return FALSE;

        if (this->attribute_id < n.attribute_id) return TRUE;
        return FALSE;
    }
};

// MAP containing the MFI TLV information
typedef std::map<firmware_info_key_t, vtss_appl_firmware_image_status_tlv_t> tlv_map_t;

// Getting the whole map with all the MFI TLV information.
mesa_rc firmware_image_tlv_map_get(tlv_map_t *tlv_map_i);

struct FirmwareBuf {
    FirmwareBuf() : size(0), data(nullptr) {}

    FirmwareBuf(size_t s) : size(s) {
        data = (char *)malloc(size);
        if (!data) size = 0;
    }

    FirmwareBuf(const FirmwareBuf &) = delete;
    FirmwareBuf &operator=(const FirmwareBuf &) = delete;

    FirmwareBuf(FirmwareBuf &&rhs) : size(rhs.size), data(rhs.data) {
        rhs.data = nullptr;
        rhs.size = 0;
    }

    FirmwareBuf &operator=(FirmwareBuf &&rhs) {
        if (data) free(data);
        data = rhs.data;
        size = rhs.size;
        rhs.data = 0;
        rhs.size = 0;
        return *this;
    }

    void clear() {
        if (data) free(data);
        data = nullptr;
        size = 0;
    }

    ~FirmwareBuf() {
        if (data) free(data);
    }

    explicit operator bool() const { return data; }

    size_t size;
    char *data;
};

#endif // _FIRMWARE_INFO_API_H_

