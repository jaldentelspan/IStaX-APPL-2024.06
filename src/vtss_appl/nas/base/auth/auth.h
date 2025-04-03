/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _AUTH_H_
#define _AUTH_H_

#include "../common/nas.h"
#include "auth_sm.h"

#define NAS_IEEE8021X_EAPOL_VERSION 2
#define AUTH_SM(sm) ((sm)->u.auth_sm)

enum {
    NAS_IEEE8021X_TYPE_EAP_PACKET                   = 0,
    NAS_IEEE8021X_TYPE_EAPOL_START                  = 1,
    NAS_IEEE8021X_TYPE_EAPOL_LOGOFF                 = 2,
    NAS_IEEE8021X_TYPE_EAPOL_KEY                    = 3,
    NAS_IEEE8021X_TYPE_EAPOL_ENCAPSULATED_ASF_ALERT = 4
};

typedef enum {
    NAS_IEEE8021X_TIMEOUT_ALL_TIMEOUTS,
    NAS_IEEE8021X_TIMEOUT_EAPOL_TIMEOUT,
    NAS_IEEE8021X_TIMEOUT_SUPPLICANT_RESPONSE_TIMEOUT
} nas_ieee8021x_timeouts_t;

typedef struct {
    u8  protocol_version;
    u8  packet_type;
    u16 body_length;
    u8  *body;
} nas_ieee8021x_eapol_packet_t;

typedef struct {
    u8  eap_code;
    u8  identifier;
    u16 length;
    /* followed by (length-4) bytes of eap packet data */
} nas_ieee8021x_eap_packet_hdr_t;

#ifdef __cplusplus
extern "C" {
#endif

// Call to generate request/identity frame
void auth_tx_canned_eap(nas_sm_t *sm, BOOL success);
void auth_tx_req(nas_sm_t *sm);
void auth_start_timeout(nas_sm_t *sm, nas_ieee8021x_timeouts_t timeout);
void auth_cancel_timeout(nas_sm_t *sm, nas_ieee8021x_timeouts_t timeout);

// Called from auth_sm.c:
void auth_backend_send_response(nas_sm_t *sm);
void auth_request_identity(nas_sm_t *sm);
#ifdef __cplusplus
}
#endif
#endif /* _AUTH_H_ */
