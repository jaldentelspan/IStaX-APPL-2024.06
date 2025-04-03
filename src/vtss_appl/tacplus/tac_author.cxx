/*
 *   AUTHORIZATION
 *
 */

#include "libtacplus.h"

/***************************************************************************
The AV-pairs list depends from Cisco IOS version

char *avpair[]=
{
   "service=(*)slip|ppp|arap|shell|tty-daemon|connection|system|firewall|multilink|...",
     This attribute MUST always be included !

   "protocol=(*)lcp|ip|ipx|atalk|vines|lat|xremote|tn3270|telnet|rlogin|pad|vpdn|ftp|http|deccp|osicp|unknown|multilink",
   "cmd=(*)command, if service=shell",
     This attribute MUST be specified if service equals "shell".
     A NULL value (cmd=NULL) indicates that the shell itself is being referred to.

   "cmd-arg=(*)argument to command",
     Multiple cmd-arg attributes may be specified

   "acl=(*)access list, if service=shell and cmd=NULL",
     Used only when service=shell and cmd=NULL

   "inacl=(*)input access list",
   "outacl=(*)output access list",
   "zonelist=(*)numeric zonelist value to AppleTalk only",
   "addr=(*)network address",
   "addr-pool=(*)address pool",
   "routing=(*)true|false, routing propagated",
   "route=(*)<dst_address> <mask> [<routing_addr>]",
     MUST be of the form "<dst_address> <mask> [<routing_addr>]"

   "timeout=(*)timer for the connection (in minutes)",
     Zero - no timeout

   "idletime=(*)idle-timeout (in minutes)",
   "autocmd=(*)auto-command, service=shell and cmd=NULL",
   "noescape=(*)true|false, deny using symbol escape",
     Used only when service=shell and cmd=NULL

   "nohangup=(*)true|false, Do no disconnect after autocmd",
     Used only when service=shell and cmd=NULL

   "priv_lvl=(*)privilege level",
   "remote_user=(*)remote userid, for AUTHEN_METH_RCMD",
   "remote_host=(*)remote host, for AUTHEN_METH_RCMD",
   "callback-dialstring=(*)NULL, or a dialstring",
   "callback-line=(*)line number to use for a callback",
   "callback-rotary=(*)rotary number to use for a callback",
   "nocallback-verify=(*)not require authen after callback"
     ...

   This list can increase for new versions of Cisco IOS

   NULL - end of array

   = - mandatory argument
   * - optional argument

   Maximum length of 1 AV-pair is 255 chars
};
****************************************************************************/

#define TAC_AUTHOR_REQ_BUF_SIZE          1024
#define TAC_AUTHOR_REQ_FIXED_FIELDS_SIZE    8
struct author_request {
    uint8_t authen_method;
    uint8_t priv_lvl;
    uint8_t authen_type;
    uint8_t authen_service;
    uint8_t user_len;
    uint8_t port_len;
    uint8_t rem_addr_len;
    uint8_t arg_cnt;  /* the the number of cmd args */
    /* <arg_cnt uint8_t containing the lengths of args 1 to arg n> */
    /* <user_len bytes of char data> */
    /* <port_len bytes of char data> */
    /* <rem_addr_len bytes of char data> */
    /* <char data for each arg> */
};

/****************************************
    send request (client function)
****************************************/
int tac_author_send_request(tac_session_t *const session,
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
    uint8_t  buf[TAC_AUTHOR_REQ_BUF_SIZE] = {0};
    HDR      *hdr = (HDR *)buf;
    uint32_t length;

    struct author_request *auth = (struct author_request *)(buf + TAC_PLUS_HDR_SIZE);

    size_t ulen = strlen(user);
    size_t plen = strlen(port);
    size_t alen = strlen(rem_addr);

    uint8_t *lens = buf + TAC_PLUS_HDR_SIZE + TAC_AUTHOR_REQ_FIXED_FIELDS_SIZE;
    uint8_t arg_len[256]; /* Length of each arg */
    int     arglens = 0; /* Accumulated arg length */
    size_t  avpairs = 0; /* Actual number of avpairs */
    size_t  i;

    if (session == NULL) {
        tac_error("tac_author_send_request: session == NULL\n");
        return 0;
    }

    if (avpair_len > 256) {
        tac_error("%s: tac_author_send_request: More than 256 avpairs (%zu).\n", session->peer, avpair_len);
        return 0;
    }

    if (ulen > 255) {
        tac_error("%s: tac_author_send_request: Invalid user length (%zu).\n", session->peer, ulen);
        return 0;
    }

    if (plen > 255) {
        tac_error("%s: tac_author_send_request: Invalid port length (%zu).\n", session->peer, plen);
        return 0;
    }

    if (alen > 255) {
        tac_error("%s: tac_author_send_request: Invalid rem_addr length (%zu).\n", session->peer, alen);
        return 0;
    }

    hdr->version    = TAC_PLUS_VER_0;
    hdr->type       = TAC_PLUS_AUTHOR;      /* set packet type to authorization */

    /* Count length of each avpair and total length of avpair */
    for (avpairs = 0; avpairs < avpair_len; avpairs++) {
        if (avpair[avpairs] == NULL) {
            break;
        }

        size_t l = strlen(avpair[avpairs]);
        if (l > 255) {
            tac_error("%s: tac_author_send_request: Invalid avpair[%d] length (%zu).\n", session->peer, avpairs, l);
            return 0;
        }

        arg_len[avpairs] = l;
        arglens += l;
    }

    length = TAC_AUTHOR_REQ_FIXED_FIELDS_SIZE + avpairs + ulen + plen + alen + arglens;
    if ((TAC_PLUS_HDR_SIZE + length) > TAC_AUTHOR_REQ_BUF_SIZE) {
        tac_error("%s: tac_author_send_request: Total length exceeds %u bytes.\n", session->peer, TAC_AUTHOR_REQ_BUF_SIZE);
        return 0;
    }
    hdr->datalength = htonl(length);

    auth->authen_method  = (uint8_t)method;
    auth->priv_lvl       = (uint8_t)priv_lvl;
    auth->authen_type    = (uint8_t)authen_type;
    auth->authen_service = (uint8_t)authen_service;
    auth->user_len       = (uint8_t)ulen;
    auth->port_len       = (uint8_t)plen;
    auth->rem_addr_len   = (uint8_t)alen;
    auth->arg_cnt        = (uint8_t)avpairs;

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

#define TAC_AUTHOR_REPLY_FIXED_FIELDS_SIZE 6
/* An authorization reply packet */
struct author_reply {
    uint8_t  status;
    uint8_t  arg_cnt;
    uint16_t server_msg_len;
    uint16_t data_len;
    /* <arg_cnt bytes containing the lengths of arg 1 to arg n> */
    /* <server_msg_len bytes of char data> */
    /* <data_len bytes of char data> */
    /* <char data for each arg> */
};

int tac_author_get_response(tac_session_t *const session,
                            int *const single,
                            char *const server_msg,
                            const size_t server_msg_len,
                            char *const data,
                            const size_t data_len,
                            char **const avpair,
                            const size_t avpair_len)
{
    uint8_t             *buf;
    HDR                 *hdr;
    struct author_reply *rep;
    uint8_t             *rep_arg_lens;
    uint8_t             *rep_server_msg;
    uint8_t             *rep_data;
    uint8_t             *rep_arg;
    size_t              mlen;
    size_t              dlen;
    size_t              arglens = 0; /* Total length of arg 1..N */
    int                 i;
    int                 rc;

    if (session == NULL) {
        tac_error("tac_author_get_response: session == NULL\n");
        return -1;
    }

    if ((buf = tac_read_packet(session, single)) == NULL) {
        tac_error("%s: tac_author_get_response: buf == NULL\n", session->peer);
        return -1;
    }

    hdr            = (HDR *)buf;
    rep            = (struct author_reply *)(buf + TAC_PLUS_HDR_SIZE);
    mlen           = ntohs(rep->server_msg_len);
    dlen           = ntohs(rep->data_len);
    rep_arg_lens   = buf + TAC_PLUS_HDR_SIZE + TAC_AUTHOR_REPLY_FIXED_FIELDS_SIZE;
    rep_server_msg = rep_arg_lens + rep->arg_cnt;
    rep_data       = rep_server_msg + mlen;
    rep_arg        = rep_data + dlen;

    for (i = 0; i < rep->arg_cnt ; i++) { /* Calculate total length of args */
        arglens += rep_arg_lens[i];
    }

    if (hdr->type != TAC_PLUS_AUTHOR) {
        tac_error("%s: tac_author_get_response: Not an AUTHORIZATION reply\n", session->peer);
        tac_free(buf);
        return -1;
    }

    if (mlen > (server_msg_len - 1)) { /* Check for enough space including zero terminator */
        tac_error("%s: tac_author_get_response: mlen > (server_msg_len - 1) - (%zu > %zu).\n", session->peer, mlen, server_msg_len - 1);
        tac_free(buf);
        return -1;
    }

    if (dlen > (data_len - 1)) { /* Check for enough space including zero terminator */
        tac_error("%s: tac_author_get_response: dlen > (data_len - 1) - (%zu > %zu).\n", session->peer, dlen, data_len - 1);
        tac_free(buf);
        return -1;
    }

    if (rep->arg_cnt > (avpair_len - 1)) { /* Check for enough space including terminating NULL pointer */
        tac_error("%s: tac_author_get_response: rep->arg_cnt > (avpair_len - 1) - (%zu > %zu).\n", session->peer, rep->arg_cnt, avpair_len - 1);
        tac_free(buf);
        return -1;
    }

    if (hdr->datalength != htonl(TAC_AUTHOR_REPLY_FIXED_FIELDS_SIZE + rep->arg_cnt + mlen + dlen + arglens)) {
        tac_error("%s: tac_author_get_response: Invalid AUTHORIZATION reply packet. Check keys.\n", session->peer);
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

    for (i = 0; i < rep->arg_cnt ; i++) {
        uint8_t len = rep_arg_lens[i];
        avpair[i] = (char *)tac_malloc(len + 1); /* Make room for '\0' */
        if (avpair[i]) {
            memcpy(avpair[i], rep_arg, len);
            avpair[i][len] = '\0';
            rep_arg += len; /* Next arg */
        } else {
            tac_error("%s: tac_author_get_response: Out of memory\n", session->peer);
            return -1;
        }
    }
    avpair[i] = NULL;

    rc = rep->status;
    tac_free(buf);
    return rc;
}
