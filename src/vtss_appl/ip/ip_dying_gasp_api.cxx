/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>

#include "main.h"
#include "ip_trace.h"
#include "vtss_netlink.hxx"
#include "vtss/basics/trace.hxx"
#include "ip_dying_gasp_api.hxx"

#define D VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_DYING_GASP, DEBUG)
#define I VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_DYING_GASP, INFO)
#define N VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_DYING_GASP, NOISE)
#define E VTSS_TRACE(VTSS_MODULE_ID_IP, IP_TRACE_GRP_DYING_GASP, ERROR)

namespace vtss
{
namespace appl
{
namespace ip
{
namespace dying_gasp
{

/* kernel space definitions - must be kept in sync ! */
enum {
    VTSS_DYING_GASP_ATTR_NONE,
    VTSS_DYING_GASP_ATTR_ID,
    VTSS_DYING_GASP_ATTR_INTERFACE,
    VTSS_DYING_GASP_ATTR_MSG,
    VTSS_DYING_GASP_ATTR_END,
};

/* kernel space definitions - must be kept in sync ! */
enum {
    VTSS_DYING_GASP_GENL_BUF_ADD,
    VTSS_DYING_GASP_GENL_BUF_MODIFY,
    VTSS_DYING_GASP_GENL_BUF_DELETE,
    VTSS_DYING_GASP_GENL_BUF_DELETE_ALL,

    /* add new operations here, and remember to update user-space applications */
};

static const char *module_name = "vtss_dying_gasp";

static int id = 0;
static int dying_gasp_channel_id(void)
{
    if (id != 0 && id != -1) {
        return id;
    }

    id = netlink::genelink_channel_by_name(module_name, __FUNCTION__);
    if (id == -1) {
        E << "Failed to get channel for " << module_name;
    }

    return id;
}

#define DO(FUNC, ...)                                    \
    {                                                    \
        rc = FUNC(__VA_ARGS__);                          \
        if (rc != VTSS_RC_OK) {                          \
            I << "Failed: " #FUNC " error code: " << rc; \
            return rc;                                   \
        }                                                \
    }

#define ADD(T, TT, ...)                                                         \
    {                                                                           \
        DO(netlink::attr_add_##TT, &req.n, req.max_size, T, __VA_ARGS__)        \
    }

struct vtss_dying_gasp_req {
    vtss_dying_gasp_req(int cmd)
    {
        n.nlmsg_seq = netlink::netlink_seq();
        n.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
        n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
        n.nlmsg_type = dying_gasp_channel_id();
        g.cmd = cmd;
        g.version = 0;
    }

    // ORDERING ARE IMPORTANT!!! This structure is being casted to a "struct
    // nlmsghdr" plus payload...
    static constexpr uint32_t max_size = 2048;
    struct nlmsghdr n = {};
    struct genlmsghdr g = {};
    char attr[max_size];
};

struct CaptureId : public netlink::NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n)
    {
        struct genlmsghdr *genl;
        int len = n->nlmsg_len;

        genl = (struct genlmsghdr *)NLMSG_DATA(n);
        len = n->nlmsg_len - NLMSG_LENGTH(sizeof(*genl));
        if (len < 0) {
            E << "Msg too short for this type!";
            return;
        }

        struct rtattr *rta = GENL_RTA(genl);
        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case VTSS_DYING_GASP_ATTR_ID: {
                if (RTA_PAYLOAD(rta) != 4) {
                    E << "Unexpected size";
                    break;
                }

                id = *(int *)RTA_DATA(rta);
                ok = true;
                break;
            }
            }

            rta = RTA_NEXT(rta, len);
        }
    }

    bool ok = false;
    uint32_t id;
};

mesa_rc vtss_dying_gasp_add(const char *interface,
                            const u8 *msg,
                            size_t size,
                            int *id)
{
    CaptureId capture;
    mesa_rc rc = VTSS_RC_OK;
    vtss_dying_gasp_req req(VTSS_DYING_GASP_GENL_BUF_ADD);

    if (interface) {
        ADD(VTSS_DYING_GASP_ATTR_INTERFACE, str, interface);
    }

    if (msg) {
        ADD(VTSS_DYING_GASP_ATTR_MSG, binary, msg, size);
    }

    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq,
       __FUNCTION__, &capture);

    if (!capture.ok) {
        E << "No ID returned";
        return VTSS_RC_ERROR;
    }

    if (id) {
        *id = capture.id;
    }

    return rc;
}

mesa_rc vtss_dying_gasp_modify(int id,
                               const u8 *msg)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_dying_gasp_req req(VTSS_DYING_GASP_GENL_BUF_MODIFY);
    ADD(VTSS_DYING_GASP_ATTR_ID, u32, id);
    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq, __FUNCTION__);
    return rc;
}

mesa_rc vtss_dying_gasp_delete(int id)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_dying_gasp_req req(VTSS_DYING_GASP_GENL_BUF_DELETE);
    ADD(VTSS_DYING_GASP_ATTR_ID, u32, id);
    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq, __FUNCTION__);
    return rc;
}

mesa_rc vtss_dying_gasp_delete_all(void)
{
    mesa_rc rc = VTSS_RC_OK;
    vtss_dying_gasp_req req(VTSS_DYING_GASP_GENL_BUF_DELETE_ALL);
    DO(netlink::genl_req, (const void *)&req, req.n.nlmsg_len, req.n.nlmsg_seq, __FUNCTION__);
    return rc;
}

}  /* namespace dying_gasp */
}  /* namespace ip */
}  /* namespace appl */
}  /* namespace vtss */

