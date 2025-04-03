/*
 *   ACCOUNTING
 *
 */

#include "libtacplus.h"

/***************************************************************************
The AV-pairs list depends from Cisco IOS version

char *avpair[]=
  "task_id="
  "start_time="
  "stop_time="
  "elapsed_time="
  "timezone="
  "event=net_acct|cmd_acct|conn_acct|shell_acct|sys_acct|clock_change"
       Used only when "service=system"
  "reason="  - only for event attribute
  "bytes="
  "bytes_in="
  "bytes_out="
  "paks="
  "paks_in="
  "paks_out="
  "status="
    . . .
     The numeric status value associated with the action. This is a signed
     four (4) byte word in network byte order. 0 is defined as success.
     Negative numbers indicate errors. Positive numbers indicate non-error
     failures. The exact status values may be defined by the client.
  "err_msg="

   NULL - end of array

   Maximum length of 1 AV-pair is 255 chars
};
****************************************************************************/

#define TAC_ACCT_REQ_BUF_SIZE           1024
#define TAC_ACCT_REQ_FIXED_FIELDS_SIZE     9
struct acct_request {
    uint8_t flags;
    uint8_t authen_method;
    uint8_t priv_lvl;
    uint8_t authen_type;
    uint8_t authen_service;
    uint8_t user_len;
    uint8_t port_len;
    uint8_t rem_addr_len;
    uint8_t arg_cnt; /* the number of cmd args */
    /* one uint8_t containing size for each arg */
    /* <user_len bytes of char data> */
    /* <port_len bytes of char data> */
    /* <rem_addr_len bytes of char data> */
    /* char data for args 1 ... n */
};

int tac_account_send_request(tac_session_t *const session,
                             const int flag,
                             const int method,
                             const int priv_lvl,
                             const int authen_type,
                             const int authen_service,
                             const char *user,
                             const char *port,
                             const char *rem_addr,
                             const char *const *const avpair,
                             const size_t avpair_len)
{
    uint8_t  buf[TAC_ACCT_REQ_BUF_SIZE] = {0};
    HDR      *hdr = (HDR *)buf;
    uint32_t length;

    struct acct_request *acc = (struct acct_request *)(buf + TAC_PLUS_HDR_SIZE);

    size_t ulen = strlen(user);
    size_t plen = strlen(port);
    size_t alen = strlen(rem_addr);

    uint8_t *lens = buf + TAC_PLUS_HDR_SIZE + TAC_ACCT_REQ_FIXED_FIELDS_SIZE; /* address of arg 1 len */
    uint8_t arg_len[256]; /* Length of each arg */
    int     arglens = 0; /* Accumulated arg length */
    size_t  avpairs = 0; /* Actual number of avpairs */
    size_t  i;

    if (session == NULL) {
        tac_error("tac_account_send_request: session == NULL\n");
        return 0;
    }

    if (avpair_len > 256) {
        tac_error("%s: tac_account_send_request: More than 256 avpairs (%zu).\n", session->peer, avpair_len);
        return 0;
    }

    if (ulen > 255) {
        tac_error("%s: tac_account_send_request: Invalid user length (%zu).\n", session->peer, ulen);
        return 0;
    }

    if (plen > 255) {
        tac_error("%s: tac_account_send_request: Invalid port length (%zu).\n", session->peer, plen);
        return 0;
    }

    if (alen > 255) {
        tac_error("%s: tac_account_send_request: Invalid rem_addr length (%zu).\n", session->peer, alen);
        return 0;
    }

    hdr->version    = TAC_PLUS_VER_0;
    hdr->type       = TAC_PLUS_ACCT;

    for (avpairs = 0; avpairs < avpair_len; avpairs++) {
        if (avpair[avpairs] == NULL) {
            break;
        }

        size_t l = strlen(avpair[avpairs]);
        if (l > 255) {
            tac_error("%s: tac_account_send_request: Invalid avpair[%d] length (%zu).\n", session->peer, avpairs, l);
            return 0;
        }
        arg_len[avpairs] = l;
        arglens += l;
    }

    length = TAC_ACCT_REQ_FIXED_FIELDS_SIZE + avpairs + ulen + plen + alen + arglens;
    if ((TAC_PLUS_HDR_SIZE + length) > TAC_ACCT_REQ_BUF_SIZE) {
        tac_error("%s: tac_account_send_request: Total length exceeds %u bytes.\n", session->peer, TAC_ACCT_REQ_BUF_SIZE);
        return 0;
    }
    hdr->datalength = htonl(length);

    acc->flags          = (uint8_t)flag;
    acc->authen_method  = (uint8_t)method;
    acc->priv_lvl       = (uint8_t)priv_lvl;
    acc->authen_type    = (uint8_t)authen_type;
    acc->authen_service = (uint8_t)authen_service;
    acc->user_len       = ulen;
    acc->port_len       = plen;
    acc->rem_addr_len   = alen;
    acc->arg_cnt        = (uint8_t)avpairs;

    for (i = 0; i < avpairs; i++) {
        *lens = arg_len[i];
        lens += 1;
    }

    if (ulen) {
        memcpy(lens, user, ulen);
        lens += ulen;
    }

    if (plen) {
        memcpy(lens, port, plen);
        lens += plen;
    }

    if (alen) {
        memcpy(lens, rem_addr, alen);
        lens += alen;
    }

    for (i = 0; i < avpairs; i++) {
        memcpy(lens, avpair[i], arg_len[i]);
        lens += arg_len[i];
    }

    session->seq_no = 0; /* restart sequence number */
    return tac_write_packet(session, buf);
}

#define TAC_ACCT_REPLY_FIXED_FIELDS_SIZE 5
struct acct_reply {
    uint16_t server_msg_len;
    uint16_t data_len;
    uint8_t  status;
};

int tac_account_get_reply(tac_session_t *const session,
                          int *const single,
                          char *const server_msg,
                          const size_t server_msg_len,
                          char *const data,
                          const size_t data_len)
{
    uint8_t           *buf;
    HDR               *hdr;
    struct acct_reply *rep;
    uint8_t           *rep_server_msg;
    uint8_t           *rep_data;
    size_t            mlen;
    size_t            dlen;
    int               rc;

    if (session == NULL) {
        tac_error("tac_account_get_reply: session == NULL\n");
        return -1;
    }

    if ((buf = tac_read_packet(session, single)) == NULL) {
        tac_error("%s: tac_account_get_reply: buf == NULL\n", session->peer);
        return -1;
    }

    hdr            = (HDR *)buf;
    rep            = (struct acct_reply *)(buf + TAC_PLUS_HDR_SIZE);
    mlen           = ntohs(rep->server_msg_len);
    dlen           = ntohs(rep->data_len);
    rep_server_msg = buf + TAC_PLUS_HDR_SIZE + TAC_ACCT_REPLY_FIXED_FIELDS_SIZE;
    rep_data       = rep_server_msg + mlen;

    if (hdr->type != TAC_PLUS_ACCT) {
        tac_error("%s: tac_account_get_reply: Not an ACCT reply.\n", session->peer);
        tac_free(buf);
        return -1;
    }

    if (mlen > (server_msg_len - 1)) { /* Check for enough space including zero terminator */
        tac_error("%s: tac_account_get_reply: mlen > (server_msg_len - 1) - (%zu > %zu).\n", session->peer, mlen, server_msg_len - 1);
        tac_free(buf);
        return -1;
    }

    if (dlen > (data_len - 1)) { /* Check for enough space including zero terminator */
        tac_error("%s: tac_account_get_reply: dlen > (data_len - 1) - (%zu > %zu).\n", session->peer, dlen, data_len - 1);
        tac_free(buf);
        return -1;
    }

    if (hdr->datalength != htonl(TAC_ACCT_REPLY_FIXED_FIELDS_SIZE + mlen + dlen)) {
        tac_error("%s: tac_account_get_reply: Invalid ACCOUNT reply packet. Check keys.\n", session->peer);
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
