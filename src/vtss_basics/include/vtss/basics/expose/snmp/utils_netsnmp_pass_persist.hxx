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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_UTILS_NETSNMP_PASS_PERSIST_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_UTILS_NETSNMP_PASS_PERSIST_HXX__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vtss/basics/trace_grps.hxx>
#include <vtss/basics/trace_basics.hxx>

#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/globals.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(113, VTSS_BASICS_TRACE_GRP_SNMP, X)

namespace vtss {
namespace expose {
namespace snmp {
namespace netsnmp_pass_persist {
namespace Command {
enum E {PING, GET, GETNEXT, SET};

struct Parser {
    bool operator()(const char *& b, const char * e) {
        vtss::parser::Lit ping("PING");
        vtss::parser::Lit getnext("getnext");
        vtss::parser::Lit get("get");
        vtss::parser::Lit set("set");

        if (ping(b, e))         value = PING;
        else if (getnext(b, e)) value = GETNEXT;
        else if (get(b, e))     value = GET;
        else if (set(b, e))     value = SET;
        else                    return false;

        return true;
    }

    E& get() { return value; }
    const E& get() const { return value; }

    E value;
};

bool parse(const char *b, const char *e, E &val) {
    Parser p;
    if (p(b, e)) {
        val = p.get();
        return true;
    } else {
        return false;
    }
}
}  // namespace Command

struct ParseOidSequence {
    bool operator()(const char *& b, const char * e) {
        const char * _b = b;

        vtss::parser::Lit dot(".");
        vtss::parser::Int<int32_t, 10> oid_element;
        oid_sequence.valid = 0;

        // try parse an initial dot
        dot(b, e);

        // parse the first oid-element. One oid-element is mandotory!
        if (oid_element(b, e)) {
            oid_sequence.oids[oid_sequence.valid ++] = oid_element.get();
        } else {
            goto Error;
        }

        // parse an optional sequence of following oid elements
        while (oid_sequence.valid < OidSequence::max_length &&
               Group(b, e, dot, oid_element)) {
            oid_sequence.oids[oid_sequence.valid ++] = oid_element.get();
        }

        // Check for overflow. The while loop above will not consume more oid's
        // then it have space for. But this does not mean the there are no more.
        if (oid_sequence.valid == OidSequence::max_length &&
               Group(b, e, dot, oid_element)) {
            goto Error;
        }

        return true;

      Error:
        b = _b;
        return false;
    }

    const OidSequence& get() const { return oid_sequence; }
    OidSequence& get() { return oid_sequence; }

    OidSequence oid_sequence;
};


bool read_line(int fd, vtss::ostreamBuf &os) {
    char c;
    os.clear();

    while (read(fd, &c, 1) == 1) {
        if (c == '\n')
            return true;

        os.push(c);
    }

    return false;
}

int get_fd(int port) {
    int fd;
    int one = 1;

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof serveraddr);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        TRACE(NOISE) <<
            "Failed to create socket";
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
        TRACE(NOISE) <<
            "Failed to set socket option";
        goto Error;
    }

    if (bind(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        TRACE(NOISE) <<
            "Failed to bind";
        goto Error;
    }

    if (listen(fd, 10) == -1) {
        goto Error;
    }

    return fd;

  Error:
    close(fd);
    return -1;
}

int get_fd() {
    for (int port = 5000; port < 10000; ++port) {
        int fd = get_fd(port);

        if (fd >= 0) {
            TRACE(INFO) <<
                "Bound to port: " << port;
            return fd;
        }
    }

    TRACE(ERROR) <<
        "Failed to get a socket";
    return -1;
}

void handle_client(int fd) {
    BufStream<SBuf512> buf;
    vtss::fdstream xx(fd);

    BufStream<SBuf512> fds;

    while (read_line(fd, buf)) {
        const char *b;
        ParseOidSequence oid_sequence_parser;

        TRACE(NOISE) <<
            "Got line: " << buf;

        Command::E command;
        if (!Command::parse(buf.begin(), buf.end(), command)) {
            TRACE(ERROR) <<
                "Syntax error. Did not understand: " << buf;
            continue;
        }

        switch (command) {
            case Command::PING:
                break;

            case Command::GET:
            case Command::GETNEXT:
            case Command::SET:
                read_line(fd, buf);
                TRACE(NOISE) <<
                    "Got line: " << buf;

                b = buf.begin();
                if (!oid_sequence_parser(b, buf.end())) {
                    TRACE(ERROR) <<
                        "Syntax error. Did not understand: " << buf;
                    continue;
                }
                break;
        }

        switch (command) {
            case Command::PING:
                TRACE(DEBUG) <<
                    "Handling ping " << oid_sequence_parser.get();
                fds << "PONG\n";
                break;

            case Command::GET:
                {
                    OidSequence o = oid_sequence_parser.get();
                    TRACE(DEBUG) <<
                        "Handling get " << o;

                    ErrorCode::E e = vtss_snmp_globals.get(o, &fds);

                    if (e != ErrorCode::noError) {
                        TRACE(DEBUG) <<
                            "Get request failed: " << e;
                        fds << "NONE\n";
                    }
                }
                break;

            case Command::GETNEXT:
                {
                    OidSequence i = oid_sequence_parser.get();
                    OidSequence o = oid_sequence_parser.get();
                    TRACE(DEBUG) <<
                        "Handling getnext " << o;

                    ErrorCode::E e = vtss_snmp_globals.get_next(i, o, &fds);

                    if (e != ErrorCode::noError) {
                        TRACE(DEBUG) <<
                            "No answer";
                        fds << "NONE\n";
                    }
                }
                break;

            case Command::SET:
                {
                    read_line(fd, buf);

                    OidSequence o = oid_sequence_parser.get();
                    TRACE(DEBUG) <<
                        "Handling set " << o << " -> " << buf;

                    ErrorCode::E e = vtss_snmp_globals.set(
                            o, str(buf.begin(), buf.end()), &fds);

                    if (e != ErrorCode::noError)
                        TRACE(DEBUG) <<
                            "Set request failed: " << e;

                    fds << "OK\n";
                }
                break;
        }

        xx << fds;
        fds.clear();
    }
}

}  // namespace netsnmp_pass_persist
}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#undef TRACE
#endif  // __VTSS_BASICS_EXPOSE_SNMP_UTILS_NETSNMP_PASS_PERSIST_HXX__
