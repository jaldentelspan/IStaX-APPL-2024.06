/*
 *   AUTHENTICATION
 *
 */

#include "libtacplus.h"

#define TAC_AUTHEN_REQ_BUF_SIZE            1024
#define TAC_AUTHEN_START_FIXED_FIELDS_SIZE    8
struct authen_start {
    uint8_t action;
    uint8_t priv_lvl;
    uint8_t authen_type;
    uint8_t service;
    uint8_t user_len;
    uint8_t port_len;
    uint8_t rem_addr_len;
    uint8_t data_len;
    /* <user_len bytes of char data> */
    /* <port_len bytes of char data> */
    /* <rem_addr_len bytes of uint8_t data> */
    /* <data_len bytes of uint8_t data> */
};

int tac_authen_send_start(tac_session_t *const session,
                          const int type,
                          const char *user,
                          const char *port,
                          const char *rem_addr,
                          const char *data)
{
    uint8_t buf[TAC_AUTHEN_REQ_BUF_SIZE] = {0};
    HDR     *hdr = (HDR *)buf;
    u_int   length;

    struct authen_start *ask = (struct authen_start *)(buf + TAC_PLUS_HDR_SIZE);

    uint8_t *u   = buf + TAC_PLUS_HDR_SIZE + TAC_AUTHEN_START_FIXED_FIELDS_SIZE;
    size_t  ulen = strlen(user);

    uint8_t *p   = u + ulen;
    size_t  plen = strlen(port);

    uint8_t *a   = p + plen;
    size_t  alen = strlen(rem_addr);

    uint8_t *d   = a + alen;
    size_t  dlen = strlen(data);

    if (session == NULL) {
        tac_error("tac_authen_send_start: session == NULL\n");
        return 0;
    }

    if (ulen > 255) {
        tac_error("%s: tac_authen_send_start: Invalid user length (%zu).\n", session->peer, ulen);
        return 0;
    }

    if (plen > 255) {
        tac_error("%s: tac_authen_send_start: Invalid port length (%zu).\n", session->peer, plen);
        return 0;
    }

    if (alen > 255) {
        tac_error("%s: tac_authen_send_start: Invalid rem_addr length (%zu).\n", session->peer, alen);
        return 0;
    }

    if (dlen > 255) {
        tac_error("%s: tac_authen_send_start: Invalid data length (%zu).\n", session->peer, dlen);
        return 0;
    }

    length = TAC_AUTHEN_START_FIXED_FIELDS_SIZE + ulen + plen + alen;
    if (type == TACACS_CHAP_LOGIN || type == TACACS_MSCHAP_LOGIN) {
        length += 1 + dlen;
    } else if (type == TACACS_PAP_LOGIN || type == TACACS_ARAP_LOGIN) {
        length += dlen;
    }

    if ((length + TAC_PLUS_HDR_SIZE) > TAC_AUTHEN_REQ_BUF_SIZE) {
        tac_error("%s: tac_authen_send_start: Total length exceeds %u bytes.\n", session->peer, TAC_AUTHEN_REQ_BUF_SIZE);
        return 0;
    }

    if (type == TACACS_ENABLE_REQUEST || type == TACACS_ASCII_LOGIN) {
        hdr->version = TAC_PLUS_VER_0;
    } else {
        hdr->version = TAC_PLUS_VER_1;
    }

    hdr->type       = TAC_PLUS_AUTHEN;
    hdr->datalength = htonl(length);

    ask->priv_lvl = TAC_PLUS_PRIV_LVL_MIN;

    switch (type) {
    case TACACS_ENABLE_REQUEST:
        ask->action = TAC_PLUS_AUTHEN_LOGIN;
        ask->service = TAC_PLUS_AUTHEN_SVC_ENABLE;
        break;
    case TACACS_ASCII_LOGIN:
        ask->action = TAC_PLUS_AUTHEN_LOGIN;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_ASCII;
        ask->service = TAC_PLUS_AUTHEN_SVC_LOGIN;
        break;
    case TACACS_PAP_LOGIN:
        ask->action = TAC_PLUS_AUTHEN_LOGIN;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_PAP;
        break;
    case TACACS_PAP_OUT:
        ask->action = TAC_PLUS_AUTHEN_SENDAUTH;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_PAP;
        break;
    case TACACS_CHAP_LOGIN:
        ask->action = TAC_PLUS_AUTHEN_LOGIN;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_CHAP;
        break;
    case TACACS_CHAP_OUT:
        ask->action = TAC_PLUS_AUTHEN_SENDAUTH;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_CHAP;
        break;
    case TACACS_MSCHAP_LOGIN:
        ask->action = TAC_PLUS_AUTHEN_LOGIN;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_MSCHAP;
        break;
    case TACACS_MSCHAP_OUT:
        ask->action = TAC_PLUS_AUTHEN_SENDAUTH;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_MSCHAP;
        break;
    case TACACS_ARAP_LOGIN:
        ask->action = TAC_PLUS_AUTHEN_LOGIN;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_ARAP;
        break;
    case TACACS_ASCII_CHPASS:
        ask->action = TAC_PLUS_AUTHEN_CHPASS;
        ask->authen_type = TAC_PLUS_AUTHEN_TYPE_ASCII;
        break;
    }

    ask->user_len = ulen;
    if (ulen) {
        memcpy(u, user, ulen);
    }

    ask->port_len = plen;
    if (plen) {
        memcpy(p, port, plen);
    }

    ask->rem_addr_len = alen;
    if (alen) {
        memcpy(a, rem_addr, alen);
    }

    ask->data_len = dlen;
    if (type == TACACS_CHAP_LOGIN) {
        *d++ = 1;
        if (dlen) {
            memcpy(d, data, dlen);
        }
    }
    if (type == TACACS_ARAP_LOGIN || type == TACACS_PAP_LOGIN) {
        if (dlen) {
            memcpy(d, data, dlen);
        }
    }

    session->seq_no = 0; /* restart sequence number */
    return tac_write_packet(session, buf);
}

#define TAC_AUTHEN_REPLY_FIXED_FIELDS_SIZE 6
struct authen_reply {
    uint8_t  status;
    uint8_t  flags;
#define TAC_PLUS_AUTHEN_FLAG_NOECHO   0x01
    uint16_t server_msg_len;
    uint16_t data_len;
    /* <msg_len bytes of char data> */
    /* <data_len bytes of uint8_t data> */
};

int tac_authen_get_reply(tac_session_t *const session,
                         int *const single,
                         char *const server_msg,
                         const size_t server_msg_len,
                         char *const data,
                         const size_t data_len)
{
    uint8_t             *buf;
    HDR                 *hdr;
    struct authen_reply *rep;
    uint8_t             *rep_server_msg;
    uint8_t             *rep_data;
    size_t              mlen;
    size_t              dlen;
    int                 rc;

    if (session == NULL) {
        tac_error("tac_authen_get_reply: session == NULL\n");
        return -1;
    }

    if ((buf = tac_read_packet(session, single)) == NULL) {
        tac_error("%s: tac_authen_get_reply: buf == NULL\n", session->peer);
        return -1;
    }

    hdr            = (HDR *)buf;
    rep            = (struct authen_reply *)(buf + TAC_PLUS_HDR_SIZE);
    mlen           = ntohs(rep->server_msg_len);
    dlen           = ntohs(rep->data_len);
    rep_server_msg = buf + TAC_PLUS_HDR_SIZE + TAC_AUTHEN_REPLY_FIXED_FIELDS_SIZE;
    rep_data       = rep_server_msg + mlen;

    if (hdr->type != TAC_PLUS_AUTHEN) {
        tac_error("%s: tac_authen_get_reply: Not an AUTHENTICATION reply.\n", session->peer);
        tac_free(buf);
        return -1;
    }

    if (mlen > (server_msg_len - 1)) { /* Check for enough space including zero terminator */
        tac_error("%s: tac_authen_get_reply: mlen > (server_msg_len - 1) - (%zu > %zu).\n", session->peer, mlen, server_msg_len - 1);
        tac_free(buf);
        return -1;
    }

    if (dlen > (data_len - 1)) { /* Check for enough space including zero terminator */
        tac_error("%s: tac_authen_get_reply: dlen > (data_len - 1) - (%zu > %zu).\n", session->peer, dlen, data_len - 1);
        tac_free(buf);
        return -1;
    }

    if (hdr->datalength != htonl(TAC_AUTHEN_REPLY_FIXED_FIELDS_SIZE + mlen + dlen)) {
        tac_error("%s: tac_authen_get_reply: Invalid AUTHENTICATION reply packet. Check keys.\n", session->peer);
        tac_free(buf);
        return -1;
    }

    if (mlen) {
        memcpy(server_msg, rep_server_msg, mlen);
    }
    server_msg[mlen] = '\0';

    if (dlen) {
        memcpy(data, rep_data, dlen);
    }
    data[dlen] = '\0';

    rc = rep->status;
    tac_free(buf);
    return rc;
}

#define TAC_AUTHEN_CONT_FIXED_FIELDS_SIZE 5
struct authen_cont {
    uint16_t user_msg_len;
    uint16_t data_len;
    uint8_t  flags;
#define TAC_PLUS_CONTINUE_FLAG_ABORT 0x1
    /* <user_msg_len bytes of uint8_t data> */
    /* <data_len bytes of uint8_t data> */
};

int tac_authen_send_cont(tac_session_t *const session,
                         const char *user_msg,
                         const char *data)
{
    uint8_t buf[TAC_AUTHEN_REQ_BUF_SIZE] = {0};
    u_int   length;
    HDR     *hdr = (HDR *)buf;

    struct authen_cont *ask = (struct authen_cont *)(buf + TAC_PLUS_HDR_SIZE);

    uint8_t *u   = buf + TAC_PLUS_HDR_SIZE + TAC_AUTHEN_CONT_FIXED_FIELDS_SIZE;
    size_t  ulen = strlen(user_msg);

    uint8_t *d   = u + ulen;
    size_t  dlen = strlen(data);

    if (session == NULL) {
        tac_error("tac_authen_send_cont: session == NULL\n");
        return 0;
    }

    length = TAC_AUTHEN_CONT_FIXED_FIELDS_SIZE + ulen + dlen;
    if ((length + TAC_PLUS_HDR_SIZE) > TAC_AUTHEN_REQ_BUF_SIZE) {
        tac_error("%s: tac_authen_send_cont: Length > %u.\n", session->peer, TAC_AUTHEN_REQ_BUF_SIZE);
        return 0;
    }

    hdr->version    = TAC_PLUS_VER_0;
    hdr->type       = TAC_PLUS_AUTHEN;
    hdr->datalength = htonl(length);

    ask->user_msg_len = htons(ulen);
    if (ulen) {
        memcpy(u, user_msg, ulen);
    }

    ask->data_len = htons(dlen);
    if (dlen) {
        memcpy(d, data, dlen);
    }

    return tac_write_packet(session, buf);
}
