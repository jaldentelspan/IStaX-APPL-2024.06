/*!
 * \file
 * \brief TACACS+ API
 * \details This header file describes TACACS+ definitions, types and functions.
 */

#include <sys/types.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include "tac_callout.h"

#ifndef __LIBTACACS_H__
#define __LIBTACACS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MD5_LEN                  16

#define MSCHAP_DIGEST_LEN        49

#ifndef TAC_PLUS_PORT
#define TAC_PLUS_PORT            49
#endif

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define TAC_PLUS_READ_TIMEOUT     5 /* seconds */
#define TAC_PLUS_WRITE_TIMEOUT    5 /* seconds */

/* types of authentication - used as type parameter in tac_auth_send_start() */
#define TACACS_ENABLE_REQUEST             1  /* Enable Requests */
#define TACACS_ASCII_LOGIN                2  /* Inbound ASCII Login */
#define TACACS_PAP_LOGIN                  3  /* Inbound PAP Login */
#define TACACS_CHAP_LOGIN                 4  /* Inbound CHAP login */
#define TACACS_ARAP_LOGIN                 5  /* Inbound ARAP login */
#define TACACS_PAP_OUT                    6  /* Outbound PAP request */
#define TACACS_CHAP_OUT                   7  /* Outbound CHAP request */
#define TACACS_ASCII_ARAP_OUT             8  /* Outbound ASCII and ARAP request */
#define TACACS_ASCII_CHPASS               9  /* ASCII change password request */
#define TACACS_PPP_CHPASS                10  /* PPP change password request */
#define TACACS_ARAP_CHPASS               11  /* ARAP change password request */
#define TACACS_MSCHAP_LOGIN              12  /* MS-CHAP inbound login */
#define TACACS_MSCHAP_OUT                13  /* MS-CHAP outbound login */

#define TAC_PLUS_AUTHEN_LOGIN            0x01
#define TAC_PLUS_AUTHEN_CHPASS           0x02
#define TAC_PLUS_AUTHEN_SENDPASS         0x03  /* deprecated */
#define TAC_PLUS_AUTHEN_SENDAUTH         0x04

/* priv_levels */
#define TAC_PLUS_PRIV_LVL_MAX            0x0f  /* 15 */
#define TAC_PLUS_PRIV_LVL_ROOT           0x0f  /* 15 */
#define TAC_PLUS_PRIV_LVL_USER           0x01
#define TAC_PLUS_PRIV_LVL_MIN            0x00

/* authen types */
#define TAC_PLUS_AUTHEN_TYPE_ASCII       0x01  /*  ascii  */
#define TAC_PLUS_AUTHEN_TYPE_PAP         0x02  /*  pap    */
#define TAC_PLUS_AUTHEN_TYPE_CHAP        0x03  /*  chap   */
#define TAC_PLUS_AUTHEN_TYPE_ARAP        0x04  /*  arap   */
#define TAC_PLUS_AUTHEN_TYPE_MSCHAP      0x05  /*  mschap */

/* authen services */
#define TAC_PLUS_AUTHEN_SVC_NONE         0x00
#define TAC_PLUS_AUTHEN_SVC_LOGIN        0x01
#define TAC_PLUS_AUTHEN_SVC_ENABLE       0x02
#define TAC_PLUS_AUTHEN_SVC_PPP          0x03
#define TAC_PLUS_AUTHEN_SVC_ARAP         0x04
#define TAC_PLUS_AUTHEN_SVC_PT           0x05
#define TAC_PLUS_AUTHEN_SVC_RCMD         0x06
#define TAC_PLUS_AUTHEN_SVC_X25          0x07
#define TAC_PLUS_AUTHEN_SVC_NASI         0x08
#define TAC_PLUS_AUTHEN_SVC_FWPROXY      0x09

/* status of reply packet, that client get from server in authen */
#define TAC_PLUS_AUTHEN_STATUS_PASS      0x01
#define TAC_PLUS_AUTHEN_STATUS_FAIL      0x02
#define TAC_PLUS_AUTHEN_STATUS_GETDATA   0x03
#define TAC_PLUS_AUTHEN_STATUS_GETUSER   0x04
#define TAC_PLUS_AUTHEN_STATUS_GETPASS   0x05
#define TAC_PLUS_AUTHEN_STATUS_RESTART   0x06
#define TAC_PLUS_AUTHEN_STATUS_ERROR     0x07
#define TAC_PLUS_AUTHEN_STATUS_FOLLOW    0x21

/* methods of authentication */
#define TAC_PLUS_AUTHEN_METH_NOT_SET     0x00
#define TAC_PLUS_AUTHEN_METH_NONE        0x01
#define TAC_PLUS_AUTHEN_METH_KRB5        0x02
#define TAC_PLUS_AUTHEN_METH_LINE        0x03
#define TAC_PLUS_AUTHEN_METH_ENABLE      0x04
#define TAC_PLUS_AUTHEN_METH_LOCAL       0x05
#define TAC_PLUS_AUTHEN_METH_TACACSPLUS  0x06
#define TAC_PLUS_AUTHEN_METH_GUEST       0x08
#define TAC_PLUS_AUTHEN_METH_RADIUS      0x10  /* 16 */
#define TAC_PLUS_AUTHEN_METH_KRB4        0x11  /* 17 */
#define TAC_PLUS_AUTHEN_METH_RCMD        0x20  /* 32 */

/* authorization status */
#define TAC_PLUS_AUTHOR_STATUS_PASS_ADD  0x01
#define TAC_PLUS_AUTHOR_STATUS_PASS_REPL 0x02
#define TAC_PLUS_AUTHOR_STATUS_FAIL      0x10  /* 16 */
#define TAC_PLUS_AUTHOR_STATUS_ERROR     0x11  /* 17 */
#define TAC_PLUS_AUTHOR_STATUS_FOLLOW    0x21  /* 33 */

/* accounting flag */
#define TAC_PLUS_ACCT_FLAG_MORE          0x01  /* deprecated */
#define TAC_PLUS_ACCT_FLAG_START         0x02
#define TAC_PLUS_ACCT_FLAG_STOP          0x04
#define TAC_PLUS_ACCT_FLAG_WATCHDOG      0x08

/* accounting status */
#define TAC_PLUS_ACCT_STATUS_SUCCESS     0x01
#define TAC_PLUS_ACCT_STATUS_ERROR       0x02
#define TAC_PLUS_ACCT_STATUS_FOLLOW      0x21  /* 33 */

/* All tacacs+ packets have the same header format */
#define TAC_PLUS_HDR_SIZE 12
struct tac_plus_hdr {
    uint8_t version;

#define TAC_PLUS_MAJOR_VER_MASK          0xf0
#define TAC_PLUS_MAJOR_VER               0xc0

#define TAC_PLUS_MINOR_VER_0             0x00
#define TAC_PLUS_VER_0                   (TAC_PLUS_MAJOR_VER | TAC_PLUS_MINOR_VER_0)

#define TAC_PLUS_MINOR_VER_1             0x01
#define TAC_PLUS_VER_1                   (TAC_PLUS_MAJOR_VER | TAC_PLUS_MINOR_VER_1)

    uint8_t type;

#define TAC_PLUS_AUTHEN                  0x01
#define TAC_PLUS_AUTHOR                  0x02
#define TAC_PLUS_ACCT                    0x03

    uint8_t seq_no;                            /* packet sequence number */
    uint8_t flags;                             /* bitmapped flags */

#define TAC_PLUS_UNENCRYPTED_FLAG        0x01  /* packet is not encrypted */
#define TAC_PLUS_SINGLE_CONNECT_FLAG     0x04  /* multiple sessions over a single connection */

    uint32_t session_id;                       /* session identifier */
    uint32_t datalength;                       /* length of encrypted data following this header */
    /* datalength bytes of encrypted data */
};
typedef struct tac_plus_hdr HDR;

struct tac_session {
    int session_id;  /* host specific unique session id */
    int aborted;     /* have we received an abort flag? */
    int seq_no;      /* seq. no. of last packet exchanged */
    int sock;        /* socket for this connection */
    char *key;       /* the key */
    char *peer;      /* name of connected peer */
    int single;      /* ask for single connection */
};
typedef struct tac_session tac_session_t;

/***************************************************************************
    INTERNAL FUNCTIONS
****************************************************************************/
/*!
 * \brief Read and decrypt a packet from TACACS+ server.
 *
 * The returned pointer must be free'd with tac_free().
 *
 * \param session [IN]  The current session.
 * \param single  [OUT] True if single connection is enabled and supported.
 *
 * \return Pointer to received packet or NULL on error.
 */
uint8_t *tac_read_packet(tac_session_t *session, int *single);

/*!
 * \brief Encrypt and send a packet to TACACS+ server.
 *
 * \param session [IN]  The current session.
 * \param pak     [IN]  Pointer to packet data to send.
 *
 * \return 1 on success, 0 on error.
 */
int tac_write_packet(tac_session_t *session, uint8_t *pak);

/***************************************************************************
    UTILITY FUNCTIONS
****************************************************************************/
void tac_error(const char *format, ...);          /* common error print function */
void tac_free_avpairs(char **avp);                /* free avpairs array */
const char *tac_print_authen_status(int status);  /* translate authenticaton reply status to string */
const char *tac_print_author_status(int status);  /* translate authorization reply status to string */
const char *tac_print_account_status(int status); /* translate accounting reply status to string */

/***************************************************************************
    CLIENT FUNCTIONS
****************************************************************************/
/*!
 * \brief Connect to TACACS+ server.
 *
 * \param peer    [IN]  Server name or IP address.
 * \param port    [IN]  Server port. If 0, use default port 49.
 * \param single  [IN]  If TRUE, ask for single connection capability.
 * \param timeout [IN]  Number of seconds to wait for connection to be established.
 * \param key     [IN]  Encryption key. If NULL don't encrypt and decrypt.
 *
 * \return Pointer to session or NULL on error.
 */
tac_session_t *tac_connect(const char *peer,
                           const int port,
                           const int single,
                           const int timeout,
                           const char *key);

/*!
 * \brief Close connection to TACACS+ server.
 *
 * \param session [IN]  The current session.
 *
 * \return Nothing.
 */
void tac_close(tac_session_t *session);

/*!
 * \brief Send authentication START packet to TACACS+ server.
 *
 * \param session  [IN]  The current session.
 * \param type     [IN]  One of TACACS_XXXXX above
 * \param user     [IN]  Username
 * \param port     [IN]  Name of client port
 * \param rem_addr [IN]  Name of remote address
 * \param data     [IN]  External data to tacacs+ server
 *
 * \return 1 on success, 0 on error.
 */
int tac_authen_send_start(tac_session_t *const session,
                          const int type,
                          const char *user,
                          const char *port,
                          const char *rem_addr,
                          const char *data);

/*!
 * \brief Get authentication REPLY packet from TACACS+ server.
 *
 * \param session        [IN]  The current session.
 * \param single         [OUT] TRUE if single connection is wanted and supported
 * \param server_msg     [OUT] Buffer for msg from server
 * \param server_msg_len [IN]  Msg buffer length
 * \param data           [OUT] Buffer for data from server
 * \param data_len       [IN]  Data buffer length
 *
 * \return TAC_PLUS_AUTHEN_STATUS_XXXXX on success, -1 on error.
 */
int tac_authen_get_reply(tac_session_t *const session,
                         int *const single,
                         char *const server_msg,
                         const size_t server_msg_len,
                         char *const data,
                         const size_t data_len);

/*!
 * \brief Send authentication CONTINUE packet to TACACS+ server.
 *
 * \param session  [IN]  The current session.
 * \param user_msg [IN]  User info to server - such as password.
 * \param data     [IN]  Data to server
 *
 * \return 1 on success, 0 on error.
 */
int tac_authen_send_cont(tac_session_t *const session,
                         const char *user_msg,
                         const char *data);

/*!
 * \brief Send authorization REQUEST packet to TACACS+ server.
 *
 * \param session        [IN]  The current session.
 * \param method         [IN]  One of TAC_PLUS_AUTHEN_METH_XXXXX above
 * \param priv_lvl       [IN]  Current privilege level
 * \param authen_type    [IN]  One of TAC_PLUS_AUTHEN_TYPE_XXXXX above
 * \param authen_service [IN]  One of TAC_PLUS_AUTHEN_SVC_XXXXX above
 * \param user           [IN]  Username
 * \param port           [IN]  Name of client port
 * \param rem_addr       [IN]  Name of remote address (if any)
 * \param avpair         [IN]  One or more attribute-value pair
 * \param avpair_len     [IN]  Avpair buffer length
 *
 * \return 1 on success, 0 on error.
 */
int tac_author_send_request(tac_session_t *const session,
                            const int method,
                            const int priv_lvl,
                            const int authen_type,
                            const int authen_service,
                            const char *user,
                            const char *port,
                            const char *rem_addr,
                            const char *const *const avpair,
                            const size_t avpair_len);

/*!
 * \brief Get authorization RESPONSE packet from TACACS+ server.
 *
 * \param session        [IN]  The current session.
 * \param single         [OUT] TRUE if single connection is wanted and supported
 * \param server_msg     [OUT] Buffer for msg from server
 * \param server_msg_len [IN]  Msg buffer length
 * \param data           [OUT] Buffer for data from server
 * \param data_len       [IN]  Data buffer length
 * \param avpair         [OUT] One or more attribute-value pair
 * \param avpair_len     [IN]  Avpair buffer length
 *
 * \return TAC_PLUS_AUTHOR_STATUS_XXXXX on success, -1 on error.
 */
int tac_author_get_response(tac_session_t *const session,
                            int *const single,
                            char *const server_msg,
                            const size_t server_msg_len,
                            char *const data,
                            const size_t data_len,
                            char **const avpair,
                            const size_t avpair_len);

/*!
 * \brief Send accounting REQUEST packet to TACACS+ server.
 *
 * \param session        [IN]  The current session.
 * \param flag           [IN]  One of TAC_PLUS_ACCT_FLAG_XXXXX above
 * \param method         [IN]  One of TAC_PLUS_AUTHEN_METH_XXXXX above
 * \param priv_lvl       [IN]  Current privilege level
 * \param authen_type    [IN]  One of TAC_PLUS_AUTHEN_TYPE_XXXXX above
 * \param authen_service [IN]  One of TAC_PLUS_AUTHEN_SVC_XXXXX above
 * \param user           [IN]  Username
 * \param port           [IN]  Name of client port
 * \param rem_addr       [IN]  Name of remote address (if any)
 * \param avpair         [IN]  One or more attribute-value pair
 * \param avpair_len     [IN]  Avpair buffer length
 *
 * \return 1 on success, 0 on error.
 */
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
                             const size_t avpair_len);

/*!
 * \brief Get accounting REPLY packet from TACACS+ server.
 *
 * \param session        [IN]  The current session.
 * \param single         [OUT] TRUE if single connection is wanted and supported
 * \param server_msg     [OUT] Buffer for msg from server
 * \param server_msg_len [IN]  Msg buffer length
 * \param data           [OUT] Buffer for data from server
 * \param data_len       [IN]  Data buffer length
 *
 * \return TAC_PLUS_ACCT_STATUS_XXXXX on success, -1 on error.
 */
int tac_account_get_reply(tac_session_t *const session,
                          int *const single,
                          char *const server_msg,
                          const size_t server_msg_len,
                          char *const data,
                          const size_t data_len);

#ifdef __cplusplus
}
#endif

#endif   /* __LIBTACACS_H__ */
