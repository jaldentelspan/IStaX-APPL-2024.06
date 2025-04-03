#include "main.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "libtacplus.h"
#include "vtss_md5_api.h"

/*lint --e{573} Suppress Lint Warning 573: Signed-unsigned mix with divide */

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_AUTH

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#else
/* Define dummy syslog macros */
#define S_W(fmt, ...)
#endif

typedef struct vtss_MD5Context MD5_CTX;
static unsigned int seedp = 1;

/* tac_connect modified for EStaX to use non-blocking socket instead of signal/alarm */
#define tac_abort(e, str)                                                             \
  do {                                                                                \
      if (session) {                                                                  \
          session->aborted = 1;                                                       \
          tac_close(session);                                                         \
      }                                                                               \
      if (res) {                                                                      \
          freeaddrinfo(res);                                                          \
      }                                                                               \
      if (verbose) {                                                                  \
          tac_error("aborted tac_connect on %s operation: %s\n", (str), strerror(e)); \
      } else {                                                                        \
          S_W("aborted tac_connect on %s operation: %s\n", (str), strerror(e));       \
      }                                                                               \
      return NULL;                                                                    \
  } while (0)

tac_session_t *tac_connect(const char *peer, const int port, const int single, const int timeout, const char *key)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    tac_session_t   *session;
    int             error;
    int             flag;
    int             verbose = 0; // redirect the error to syslog
    char            port_string[16];

    session = (tac_session_t *)tac_malloc(sizeof(tac_session_t));
    if (session == NULL) {
        verbose = 1; // Critical error, we display it on console.
        tac_abort(ENOMEM, "malloc");
    }
    memset(session, 0, sizeof(tac_session_t));
    session->aborted = 0;
    session->sock = -1;

    session->peer = tac_strdup(peer);
    if (session->peer == NULL) {
        verbose = 1; // Critical error, we display it on console.
        tac_abort(ENOMEM, "tac_strdup(peer)");
    }
    if (key) {
        session->key = tac_strdup(key);
        if (session->key == NULL) {
            verbose = 1; // Critical error, we display it on console.
            tac_abort(ENOMEM, "tac_strdup(key)");
        }
    }

    sprintf(port_string, "%d", (port == 0) ? 49 : port);

    /* setup hints structure */
    memset(&hints, 0, sizeof(hints));
#ifdef VTSS_SW_OPTION_IPV6
    hints.ai_family = AF_UNSPEC; /* Accept both IPv4 and IPv6 */
#else
    hints.ai_family = AF_INET; /* Accept IPv4 only */
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if ((error = getaddrinfo(peer, port_string, &hints, &res))) {
        // In case of an error, getaddrinfo has already called freeaddrinfo(), but it never sets 'res' to NULL.
        // Set it here in order to prevent tac_abort() to call freeaddrinfo() again with the same pointer.
        res = NULL;
        verbose = 0; // Suppress own error message here.
        tac_abort(EINVAL, gai_strerror(error));
    }
    // We will only try the first addrinfo in res and NOT traverse the whole list
    if ((session->sock = vtss_socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        tac_abort(errno, "socket");
    }

    /* Set socket to non-blocking mode */
    flag = 1;
    if (ioctl(session->sock, FIONBIO, &flag) < 0) {
        tac_abort(errno, "iocnl(...,FIONBIO) 1");
    }

    error = connect(session->sock, res->ai_addr, res->ai_addrlen);
    if (error < 0) {
        if (errno == EINPROGRESS) {
            fd_set         fdset;
            struct timeval tv;
            do {
                tv.tv_sec = timeout;
                tv.tv_usec = 0;
                FD_ZERO(&fdset);
                FD_SET(session->sock, &fdset);
                error = select(session->sock + 1, NULL, &fdset, NULL, &tv);
                if (error < 0 && errno != EINTR) {
                    tac_abort(errno, "select");
                } else if (error > 0) {
                    // select has now told us that the socket is writable, but we will need to check for errors
                    int       option_value;
                    socklen_t option_length;
                    option_length = sizeof(int);
                    if (getsockopt(session->sock, SOL_SOCKET, SO_ERROR, (void *)(&option_value), &option_length) < 0) {
                        tac_abort(errno, "getsockopt");
                    }
                    if (option_value) {
                        tac_abort(option_value, "delayed connect");
                    }
                    break; /* SUCCESS - we are now connected */
                } else {
                    tac_abort(ETIMEDOUT, "timeout");
                }
            } while (1);
        } else {
            verbose = 0; // Suppress error message here.
            tac_abort(errno, "connect");
        }
    }

    /* Set socket back to blocking mode again */
    flag = 0;
    if (ioctl(session->sock, FIONBIO, &flag) < 0) {
        tac_abort(errno, "iocnl(...,FIONBIO) 2");
    }

    freeaddrinfo(res);
    session->session_id = rand_r(&seedp); /* Generate a random session_id */
    session->seq_no = 0;
    session->single = single;

    return session;
}

void tac_close(tac_session_t *session)
{
    if (session) {
        if (session->sock >= 0 ) {
            (void)shutdown(session->sock, SHUT_RDWR);
            close(session->sock);
        }
        if (session->peer) {
            tac_free(session->peer);
        }
        if (session->key) {
            tac_free(session->key);
        }
        tac_free(session);
    }
}


/*
 * create_md5_hash(): create an md5 hash of the "session_id", "the user's
 * key", "the version number", the "sequence number", and an optional
 * 16 bytes of data (a previously calculated hash). If not present, this
 * should be NULL pointer.
 *
 * Write resulting hash into the array pointed to by "hash".
 *
 * The caller must allocate sufficient space for the resulting hash
 * (which is 16 bytes long). The resulting hash can safely be used as
 * input to another call to create_md5_hash, as its contents are copied
 * before the new hash is generated.
 */
static void create_md5_hash(int session_id, char *key, uint8_t version, uint8_t seq_no, uint8_t *prev_hash, uint8_t *hash)
{
    uint8_t *md_stream, *mdp;
    int     md_len;
    MD5_CTX mdcontext;

    md_len = sizeof(session_id) + strlen(key) + sizeof(version) + sizeof(seq_no);

    if (prev_hash) {
        md_len += MD5_LEN;
    }
    mdp = md_stream = (uint8_t *)tac_malloc(md_len);
    if (md_stream == NULL) {
        tac_error("create_md5_hash: Out of memory\n");
        return;
    }
    bcopy(&session_id, mdp, sizeof(session_id));
    mdp += sizeof(session_id);

    bcopy(key, mdp, strlen(key));
    mdp += strlen(key);

    bcopy(&version, mdp, sizeof(version));
    mdp += sizeof(version);

    bcopy(&seq_no, mdp, sizeof(seq_no));
    mdp += sizeof(seq_no);

    if (prev_hash) {
        bcopy(prev_hash, mdp, MD5_LEN);
    }
    vtss_MD5Init(&mdcontext);
    vtss_MD5Update(&mdcontext, md_stream, md_len);
    vtss_MD5Final(hash, &mdcontext);
    tac_free(md_stream);
    return;
}


/*
 * Overwrite input data with en/decrypted version by generating an MD5 hash and
 * xor'ing data with it.
 *
 * When more than 16 bytes of hash is needed, the MD5 hash is performed
 * again with the same values as before, but with the previous hash value
 * appended to the MD5 input stream.
 *
 * Return 0 on success, -1 on failure.
 */
static int md5_xor(HDR *hdr, uint8_t *data, char *key)
{
    int     i, j;
    uint8_t hash[MD5_LEN];       /* the md5 hash */
    uint8_t last_hash[MD5_LEN];  /* the last hash we generated */
    uint8_t *prev_hashp = (uint8_t *) NULL; /* pointer to last created hash */
    int     data_len;
    int     session_id;
    uint8_t version;
    uint8_t seq_no;

    data_len = ntohl(hdr->datalength);
    session_id = hdr->session_id; /* always in network order for hashing */
    version = hdr->version;
    seq_no = hdr->seq_no;

    if (!key) {
        return 0;
    }

    for (i = 0; i < data_len; i += 16) {
        create_md5_hash(session_id, key, version, seq_no, prev_hashp, hash);
#ifdef DEBUG_MD5
        tac_error("hash: session_id=%u, key=%s, version=%d, seq_no=%d\n", session_id, key, version, seq_no);
#endif
        bcopy(hash, last_hash, MD5_LEN);
        prev_hashp = last_hash;

        for (j = 0; j < 16; j++) {
            if ((i + j) >= data_len) {
                return 0;
            }
            data[i + j] ^= hash[j];
        }
    }
    return 0;
}


/* Reading n bytes from descriptor fd to array ptr with timeout t sec
 * Timeout set for each read
 *
 * Return -1 if error, eof or timeout. Else returns number of bytes read. */
static int sockread(tac_session_t *session, int fd, uint8_t *ptr, int nbytes, int timeout)
{
    int nleft, nread;
    fd_set readfds, exceptfds;
    struct timeval tout;

    if (fd == -1) {
        return -1;
    }

    tout.tv_sec = timeout;
    tout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    FD_ZERO(&exceptfds);
    FD_SET(fd, &exceptfds);

    nleft = nbytes;

    while (nleft > 0) {
        int status = select(fd + 1, &readfds, (fd_set *) NULL, &exceptfds, &tout);

        if (status == 0) {
            tac_error("%s: timeout reading fd %d\n", session->peer, fd);
            return -1;
        }
        if (status < 0) {
            if (errno == EINTR) {
                continue;
            }
            tac_error("%s: error in select %s fd %d\n", session->peer, strerror(errno), fd);
            return -1;
        }
        if (FD_ISSET(fd, &exceptfds)) {
            tac_error("%s: exception on fd %d\n", session->peer, fd);
            return -1;
        }
        if (!FD_ISSET(fd, &readfds)) {
            tac_error("%s: spurious return from select\n", session->peer);
            continue;
        }
again:
        nread = read(fd, ptr, nleft);

        if (nread < 0) {
            if (errno == EINTR) {
                goto again;
            }
            tac_error("%s: error reading fd %d nread=%d %s\n", session->peer, fd, nread, strerror(errno));
            return -1;        /* error */

        } else if (nread == 0) {
            tac_error("%s: fd %d eof (connection closed)\n", session->peer, fd);
            return -1;        /* eof */
        }
        nleft -= nread;
        if (nleft) {
            ptr += nread;
        }
    }
    return (nbytes - nleft);
}


/* Write n bytes to descriptor fd from array ptr with timeout t
 * seconds. Note the timeout is applied to each write, not for the
 * overall operation.
 *
 * Return -1 on error, eof or timeout. Otherwise return number of bytes written. */
static int sockwrite(tac_session_t *session, int fd, const uint8_t *ptr, int bytes, int timeout)
{
    int remaining, sent;
    fd_set writefds, exceptfds;
    struct timeval tout;

    if (fd == -1) {
        return -1;
    }

    sent = 0;

    tout.tv_sec = timeout;
    tout.tv_usec = 0;


    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    FD_ZERO(&exceptfds);
    FD_SET(fd, &exceptfds);

    remaining = bytes;

    while (remaining > 0) {
        int status = select(fd + 1, (fd_set *) NULL, &writefds, &exceptfds, &tout);

        if (status == 0) {
            tac_error("%s: timeout writing to fd %d\n", session->peer, fd);
            return -1;
        }
        if (status < 0) {
            tac_error("%s: error in select fd %d\n", session->peer, fd);
            return -1;
        }
        if (FD_ISSET(fd, &exceptfds)) {
            tac_error("%s: exception on fd %d\n", session->peer, fd);
            return sent;      /* error */
        }

        if (!FD_ISSET(fd, &writefds)) {
            tac_error("%s: spurious return from select\n", session->peer);
            continue;
        }
        sent = write(fd, ptr, remaining);

        if (sent <= 0) {
            tac_error("%s: error writing fd %d sent=%d\n", session->peer, fd, sent);
            return sent;      /* error */
        }
        remaining -= sent;
        ptr += sent;
    }
    return (bytes - remaining);
}

uint8_t *tac_read_packet(tac_session_t *session, int *single)
{
    HDR      hdr;
    uint8_t  *pkt, *data;
    uint32_t len;

    if (single) {
        *single = 0;
    }

    if (session == NULL) {
        tac_error("tac_read_packet: session = NULL\n");
        return NULL;
    }

    /* read the packet header */
    len = sockread(session, session->sock, (uint8_t *)&hdr, TAC_PLUS_HDR_SIZE, TAC_PLUS_READ_TIMEOUT);
    if (len != TAC_PLUS_HDR_SIZE) {
        tac_error("%s: tac_read_packet: Read %d bytes, expecting %d\n", session->peer, len, TAC_PLUS_HDR_SIZE);
        return NULL;
    }

    if (session->session_id != ntohl(hdr.session_id)) {
        tac_error("%s: tac_read_packet: Invalid session_id.\n", session->peer);
        return NULL;
    }

    session->seq_no++; /* should now equal that of incoming packet */
    if (session->seq_no != hdr.seq_no) {
        tac_error("%s: tac_read_packet: Illegal session seq # %d != packet seq # %d\n", session->peer, session->seq_no, hdr.seq_no);
        return NULL;
    }

    if ((hdr.version & TAC_PLUS_MAJOR_VER_MASK) != TAC_PLUS_MAJOR_VER) {
        tac_error("%s: tac_read_packet: Illegal major version specified: found %d wanted %d\n", session->peer, hdr.version, TAC_PLUS_MAJOR_VER);
        return NULL;
    }

    /* get memory for the packet */
    len = TAC_PLUS_HDR_SIZE + ntohl(hdr.datalength);
    if ((len < TAC_PLUS_HDR_SIZE) || (len > 0x1000)) {
        tac_error("%s: tac_read_packet: Illegal data size: %lu\n", session->peer, len);
        return NULL;
    }
    pkt = (uint8_t *)tac_malloc(len);
    if (pkt == NULL) {
        tac_error("%s: tac_read_packet: Out of memory\n", session->peer);
        return NULL;
    }

    /* copy header into the packet */
    bcopy(&hdr, pkt, TAC_PLUS_HDR_SIZE);

    /* the data start here */
    data = pkt + TAC_PLUS_HDR_SIZE;

    /* read the rest of the packet data */
    if (sockread(session, session->sock, data, ntohl(hdr.datalength), TAC_PLUS_READ_TIMEOUT) != ntohl(hdr.datalength)) {
        tac_error("%s: tac_read_packet: bad socket read\n", session->peer);
        tac_free(pkt);
        return NULL;
    }

    /* decrypt the data portion */
    if (session->key && md5_xor((HDR *)pkt, data, session->key)) {
        tac_error("%s: tac_read_packet: error decrypting data\n", session->peer);
        tac_free(pkt);
        return NULL;
    }

    if (single) {
        *single = (session->single && ((hdr.flags & TAC_PLUS_SINGLE_CONNECT_FLAG) == TAC_PLUS_SINGLE_CONNECT_FLAG));
    }

    return pkt;
}

int tac_write_packet(tac_session_t *session, uint8_t *pak)
{
    HDR     *hdr = (HDR *)pak;
    uint8_t *data;
    int     len;

    if (session == NULL) {
        tac_error("tac_write_packet: session = NULL\n");
        return 0;
    }

    hdr->seq_no     = ++session->seq_no;
    hdr->session_id = htonl(session->session_id);
    hdr->flags = 0;

    if (!session->key) {
        hdr->flags |= TAC_PLUS_UNENCRYPTED_FLAG;
    }

    if (session->single) {
        hdr->flags |= TAC_PLUS_SINGLE_CONNECT_FLAG;
    }

    len = TAC_PLUS_HDR_SIZE + ntohl(hdr->datalength);

    /* the data start here */
    data = pak + TAC_PLUS_HDR_SIZE;

    /* encrypt the data portion */
    if (session->key && md5_xor((HDR *)pak, data, session->key)) {
        tac_error("%s: tac_write_packet: error encrypting data.\n", session->peer);
        return 0;
    }

    if (sockwrite(session, session->sock, pak, len, TAC_PLUS_WRITE_TIMEOUT) != len) {
        tac_error("%s: tac_write_packet: bad socket write\n", session->peer);
        return 0;
    }

    return 1;
}
