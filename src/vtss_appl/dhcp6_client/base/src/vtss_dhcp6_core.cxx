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

#include "critd_api.h"
#if defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#include "ip_utils.hxx"
#endif /* defined(VTSS_SW_OPTION_IP) */
#ifdef VTSS_SW_OPTION_DNS
#include "ip_dns_api.h"
#endif /* VTSS_SW_OPTION_DNS */
#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"
#endif /* VTSS_SW_OPTION_SYSLOG */
#include "vtss_dhcp6_core.hxx"

namespace vtss
{
namespace dhcp6
{
#define VTSS_TRACE_MODULE_ID                    VTSS_MODULE_ID_DHCP6C
#define VTSS_ALLOC_MODULE_ID                    VTSS_MODULE_ID_DHCP6C

static critd_t                                  DHCPV6C_crit;

#define DHCPV6C_CRIT_ENTER()                    \
    critd_enter(&DHCPV6C_crit,                  \
                __FILE__, __LINE__)

#define DHCPV6C_CRIT_EXIT()                     \
    critd_exit(&DHCPV6C_crit,                   \
               __FILE__, __LINE__)

#define DHCPV6C_CRIT_ASSERT_LOCKED()            \
    critd_assert_locked(&DHCPV6C_crit,          \
                        __FILE__, __LINE__)


#define DHCP6_DUID_DIR_CPY(x, y)                memcpy((x), (y), sizeof(DeviceDuid))
#define DHCP6_DUID_H2H_CPY(x, y)                DHCP6_DUID_DIR_CPY((x), (y))
#define DHCP6_DUID_H2N_CPY(x, y)                (void) duid_assign(TRUE, (x), FALSE, (y))
#define DHCP6_DUID_N2H_CPY(x, y)                (void) duid_assign(FALSE, (x), TRUE, (y))
#define DHCP6_DUID_N2N_CPY(x, y)                DHCP6_DUID_DIR_CPY((x), (y))

BOOL duid_equal(BOOL nwo_a, const DeviceDuid *const a, BOOL nwo_b, const DeviceDuid *const b)
{
    if (a && b) {
        if ((nwo_a ? ntohs(a->duid_type) : a->duid_type) !=
            (nwo_b ? ntohs(b->duid_type) : b->duid_type)) {
            return FALSE;
        }

        switch ( (nwo_a ? ntohs(a->duid_type) : a->duid_type) ) {
        case DHCPV6_DUID_TYPE_LLT:
            if ((nwo_a ? ntohs(a->type.llt.hardware_type) : a->type.llt.hardware_type) !=
                (nwo_b ? ntohs(b->type.llt.hardware_type) : b->type.llt.hardware_type)) {
                return FALSE;
            }
            if ((nwo_a ? ntohl(a->type.llt.time) : a->type.llt.time) !=
                (nwo_b ? ntohl(b->type.llt.time) : b->type.llt.time)) {
                return FALSE;
            }
            if (memcmp(&a->type.llt.lla, &b->type.llt.lla, sizeof(mesa_mac_t))) {
                return FALSE;
            }
            break;
        case DHCPV6_DUID_TYPE_EN:
            if ((nwo_a ? ntohl(a->type.en.enterprise_number) : a->type.en.enterprise_number) !=
                (nwo_b ? ntohl(b->type.en.enterprise_number) : b->type.en.enterprise_number)) {
                return FALSE;
            }
            if ((nwo_a ? ntohl(a->type.en.id) : a->type.en.id) !=
                (nwo_b ? ntohl(b->type.en.id) : b->type.en.id)) {
                return FALSE;
            }
            break;
        case DHCPV6_DUID_TYPE_LL:
            if ((nwo_a ? ntohs(a->type.ll.hardware_type) : a->type.ll.hardware_type) !=
                (nwo_b ? ntohs(b->type.ll.hardware_type) : b->type.ll.hardware_type)) {
                return FALSE;
            }
            if (memcmp(&a->type.ll.lla, &b->type.ll.lla, sizeof(mesa_mac_t))) {
                return FALSE;
            }
            break;
        default:
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

BOOL duid_assign(BOOL nwo_a, DeviceDuid *const a, BOOL nwo_b, const DeviceDuid *const b)
{
    if (a && b) {
        if (nwo_a) {
            a->duid_type = nwo_b ? b->duid_type : htons(b->duid_type);
        } else {
            a->duid_type = nwo_b ? ntohs(b->duid_type) : b->duid_type;
        }

        switch ( (nwo_a ? ntohs(a->duid_type) : a->duid_type) ) {
        case DHCPV6_DUID_TYPE_LLT:
            if (nwo_a) {
                a->type.llt.hardware_type = nwo_b ? b->type.llt.hardware_type : htons(b->type.llt.hardware_type);
            } else {
                a->type.llt.hardware_type = nwo_b ? ntohs(b->type.llt.hardware_type) : b->type.llt.hardware_type;
            }
            if (nwo_a) {
                a->type.llt.time = nwo_b ? b->type.llt.time : htonl(b->type.llt.time);
            } else {
                a->type.llt.time = nwo_b ? ntohl(b->type.llt.time) : b->type.llt.time;
            }
            memcpy(&a->type.llt.lla, &b->type.llt.lla, sizeof(mesa_mac_t));
            break;
        case DHCPV6_DUID_TYPE_EN:
            if (nwo_a) {
                a->type.en.enterprise_number = nwo_b ? b->type.en.enterprise_number : htonl(b->type.en.enterprise_number);
            } else {
                a->type.en.enterprise_number = nwo_b ? ntohl(b->type.en.enterprise_number) : b->type.en.enterprise_number;
            }
            if (nwo_a) {
                a->type.en.id = nwo_b ? b->type.en.id : htonl(b->type.en.id);
            } else {
                a->type.en.id = nwo_b ? ntohl(b->type.en.id) : b->type.en.id;
            }
            break;
        case DHCPV6_DUID_TYPE_LL:
            if (nwo_a) {
                a->type.ll.hardware_type = nwo_b ? b->type.ll.hardware_type : htons(b->type.ll.hardware_type);
            } else {
                a->type.ll.hardware_type = nwo_b ? ntohs(b->type.ll.hardware_type) : b->type.ll.hardware_type;
            }
            memcpy(&a->type.ll.lla, &b->type.ll.lla, sizeof(mesa_mac_t));
            break;
        default:
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

namespace client
{
static std::forward_list<Interface *>                client_intfs;
static u8                                       client_intfs_max = 0;

#define DHCP6_TX_XIDX_GENERATE(x)               do {    \
    if ((x)->xidx == 0)                                 \
        if (((x)->xidx = rand()) == 0)                  \
            (x)->xidx = vtss_current_time();            \
    (x)->xidx = ((x)->xidx << 8) >> 8;                  \
} while (0)

#define DHCP6_RXMT_RAND_SUM(x, y)               do {    \
    u32 factor = VTSS_OS_MSEC2TICK(1000) / 10;          \
    if (factor > 1) factor = rand() % factor;           \
    if (rand() % 2) (x) += factor; else (x) -= factor;  \
    factor = (y) * VTSS_OS_MSEC2TICK(1000);             \
    if (factor) (x) += rand() % factor;                 \
} while (0)

#define DHCP6_RXMT_SEC_TO_TICK(secs) VTSS_OS_MSEC2TICK((secs) * 1000)

#define DHCP6_RXMT_RT_ASSIGN(x, y)              do {    \
    vtss_tick_count_t  _ts = vtss_current_time();       \
    vtss_tick_count_t  _tb = VTSS_DHCP6_MAX_LIFETIME;   \
    if (_tb - _ts > (y))                                \
        (x)->rxmit_timeout = _ts + (y);                 \
    else                                                \
        (x)->rxmit_timeout = _tb;                       \
} while (0)

#define DHCP6_RXMT_MRD_SET(x, y)                do {    \
    vtss_tick_count_t  _ts = vtss_current_time();       \
    vtss_tick_count_t  _tb = VTSS_DHCP6_MAX_LIFETIME;   \
    vtss_tick_count_t  _tt = DHCP6_RXMT_SEC_TO_TICK(y); \
    if (_tb - _ts > _tt)                                \
        (x)->max_rxmit_duration = _ts + _tt;            \
    else                                                \
        (x)->max_rxmit_duration = _tb;                  \
} while (0)

static BOOL DHCP6_build_domain_name(u16 *const o, const char *const s, char *t)
{
    u16 ofst;

    if (!o || !t) {
        return FALSE;
    }

    ofst = *o;
    if (s) {
        u8  len, idx, cnt;

        len = 0;
        while (*(s + ofst)) {
            if (len) {
                *t++ = '.';
                ++len;
            }

            if ((cnt = *(s + ofst)) > 63) {
                return FALSE;
            }
            if (len + cnt > 253) {
                return FALSE;
            }

            ++ofst;
            for (idx = 0; idx < cnt; ++idx) {
                *t++ = *(s + ofst + idx);
                ++len;
            }
            ofst += cnt;
        }

        ++ofst;  /* count \0 */
    }

    *t = 0;
    *o = ofst;
    return TRUE;
}

static BOOL DHCP6_ipstk_address_valid(
    const mesa_vid_t    *const ifidx,
    const mesa_ipv6_t   *const ifadr,
    const u32           *const ifpfx
)
{
    mesa_ip_network_t nwa;
    BOOL              ret;

    if (!ifidx || !ifadr || !ifpfx) {
        return FALSE;
    }

    ret = TRUE;
#if defined(VTSS_SW_OPTION_IP)
    nwa.prefix_size = (*ifpfx ? *ifpfx : 128);
    nwa.address.type = MESA_IP_TYPE_IPV6;
    DHCP6_ADRS_CPY(&nwa.address.addr.ipv6, ifadr);
    ret = vtss_ip_if_address_valid(*ifidx, &nwa);
#endif /* defined(VTSS_SW_OPTION_IP) */
    T_I("%s Management Address", ret ? "Process Valid" : "Bypass Invalid/Overlapped");
    return ret;
}

static mesa_rc DHCP6_ipstk_address_del(const Interface *const ifdx)
{
    mesa_ipv6_network_t network;

    DHCPV6C_CRIT_ASSERT_LOCKED();

    if (!ifdx) {
        return VTSS_RC_ERROR;
    }
    if (DHCP6_ADRS_ISZERO(&ifdx->addrs.address)) {
        return VTSS_RC_OK;
    }
    if (!DHCP6_ipstk_address_valid(&ifdx->ifidx, &ifdx->addrs.address, &ifdx->addrs.prefix_length)) {
        return VTSS_RC_ERROR;
    }
    network.address = ifdx->addrs.address;
    network.prefix_size = ifdx->addrs.prefix_length;
    return ip_dhcp6c_del(ifdx->ifidx, &network);
}

static mesa_rc DHCP6_ipstk_address_add(const Interface *const ifdx)
{
    mesa_ipv6_network_t      network;

    DHCPV6C_CRIT_ASSERT_LOCKED();

    if (!ifdx) {
        return VTSS_RC_ERROR;
    }

    if (ifdx->addrs.type == IA_TYPE_INVALID || DHCP6_ADRS_ISZERO(&ifdx->addrs.address) || ifdx->addrs.prefix_length > 128) {
        T_D("INTF->Address %s", ifdx->addrs.type == IA_TYPE_INVALID ? "INACTIVE" : "ACTIVE");
        return VTSS_RC_ERROR;
    }
    if (!DHCP6_ipstk_address_valid(&ifdx->ifidx, &ifdx->addrs.address, &ifdx->addrs.prefix_length)) {
        return VTSS_RC_ERROR;
    }
    network.address = ifdx->addrs.address;
    network.prefix_size = ifdx->addrs.prefix_length;
    return ip_dhcp6c_add(ifdx->ifidx, &network, ifdx->addrs.valid_lifetime);
}

static BOOL DETERMINE_active_server_record(Interface *const ifdx)
{
    u8  idx, sdx, prf;

    if (!ifdx) {
        return FALSE;
    }

    sdx = prf = 0;
    for (idx = 1; idx <= DHCP6_SERVER_RECORD_CNT; ++idx) {
        if (DHCP6_ADRS_ISZERO(&ifdx->srvrs[idx].addrs)) {
            continue;
        }

        if (sdx == 0) {
            prf = DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]);
            sdx = idx;
        } else {
            if (DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]) > prf) {
                prf = DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]);
                sdx = idx;
            } else if (DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]) == prf) {
                if (DHCP6_ADRS_CMP(&ifdx->srvrs[idx].addrs, &ifdx->srvrs[sdx].addrs) > 0) {
                    sdx = idx;
                }
            }
        }
    }

    T_N("Make %sSRV(%u) active", sdx ? "" : "No", sdx);
    if (sdx) {
        ifdx->server = &ifdx->srvrs[sdx];
    } else {
        ifdx->server = NULL;
    }

    return TRUE;
}

static BOOL DETERMINE_alternate_server_record(Interface *const ifdx)
{
    u8          idx, sdx, prf;
    mesa_ipv6_t *nxt;

    if (!ifdx) {
        return FALSE;
    }

    if (ifdx->server == NULL ||
        DHCP6_ADRS_ISZERO(&ifdx->server->addrs)) {
        return DETERMINE_active_server_record(ifdx);
    }

    nxt = NULL;
    sdx = prf = 0;
    for (idx = 1; idx <= DHCP6_SERVER_RECORD_CNT; ++idx) {
        if (ifdx->server == &ifdx->srvrs[idx] ||
            DHCP6_ADRS_ISZERO(&ifdx->srvrs[idx].addrs)) {
            continue;
        }

        if (DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]) > DHCP6_SERVER_RECORD_PREF_GET(ifdx->server)) {
            continue;
        }

        if (sdx == 0) {
            if (DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]) < DHCP6_SERVER_RECORD_PREF_GET(ifdx->server)) {
                prf = DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]);
                sdx = idx;
            } else if (DHCP6_ADRS_CMP(&ifdx->server->addrs, &ifdx->srvrs[idx].addrs) > 0) {
                prf = DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]);
                sdx = idx;
            }
        } else {
            if (DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]) > prf) {
                prf = DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]);
                sdx = idx;
            } else if (DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[idx]) == prf) {
                /* find the biggest addrs among those addrs smaller than active addr */
                if (DHCP6_ADRS_CMP(&ifdx->server->addrs, &ifdx->srvrs[idx].addrs) > 0) {
                    if (!nxt) {
                        nxt = &ifdx->srvrs[idx].addrs;
                        sdx = idx;
                    } else {
                        if (DHCP6_ADRS_CMP(nxt, &ifdx->srvrs[idx].addrs) < 0) {
                            nxt = &ifdx->srvrs[idx].addrs;
                            sdx = idx;
                        }
                    }
                }
            }
        }
    }

    T_D("%sFound alternate SRV(%u)", sdx ? "" : "Not", sdx);
    if (sdx) {
        ifdx->server = &ifdx->srvrs[sdx];
    }

    return sdx ? TRUE : FALSE;
}

static BOOL APPLY_server_record(Interface *const ifdx)
{
    AddrInfo        *addrs;
    ServerRecord    *candidate;

    if (!ifdx || !ifdx->server) {
        T_D("Invalid %s to Apply!",
            ifdx ? (ifdx->server ? "?" : "Server Record") : "Interface");
        return FALSE;
    }

    addrs = &ifdx->addrs;
    candidate = ifdx->server;
    if (candidate->adrs_pool.empty()) {
        T_D("Candidate provides no address");
        if (!DHCP6_ADRS_ISZERO(&addrs->address)) {
            T_D("Mark interface address invalid");
            addrs->type = IA_TYPE_INVALID;
        }

        return TRUE;
    }

    DHCP6_WALK_FWD_LIST(candidate->adrs_pool, iter) {
        if (DHCP6_ADRS_ISZERO(&(*iter).address)) {
            continue;
        }

        if ((*iter).type == IA_TYPE_INVALID) {
            if (DHCP6_ADRS_EQUAL(&addrs->address, &(*iter).address)) {
                DHCPV6C_CRIT_ENTER();
                if (DHCP6_ipstk_address_del(ifdx) == VTSS_RC_OK) {
                    addrs->type = IA_TYPE_INVALID;
                }
                DHCPV6C_CRIT_EXIT();
            }

            continue;
        }

        *addrs = (*iter);
        addrs->refresh_ts = vtss_current_time();
        break;
    }

    return TRUE;
}

static BOOL CPY_server_record(ServerRecord *const dst, const ServerRecord *const src, BOOL inc_adr, BOOL inc_dns)
{
    if (!dst || !src) {
        return FALSE;
    }

    if (inc_dns && !dst->name_server.empty()) {
        dst->name_server.clear();
    }
    if (inc_dns && !dst->name_list.empty()) {
        dst->name_list.clear();
    }
    if (inc_adr && !dst->adrs_pool.empty()) {
        dst->adrs_pool.clear();
    }

    DHCP6_ADRS_CPY(&dst->addrs, &src->addrs);
    DHCP6_ADRS_CPY(&dst->ucast, &src->ucast);
    dst->has_unicast = src->has_unicast;
    dst->rapid_commit = src->rapid_commit;
    DHCP6_DUID_DIR_CPY(&dst->duid, &src->duid);
    dst->xid_prf = src->xid_prf;
    dst->reserved = 0;

    if (inc_dns && !src->name_server.empty()) {
        dst->name_server = src->name_server;
        DHCP6_WALK_FWD_LIST(dst->name_server, iter) {
            T_N("NSRV: %2X%2X:%2X%2X:%2X%2X:%2X%2X:%2X%2X:%2X%2X:%2X%2X:%2X%2X",
                (*iter).addr[0], (*iter).addr[1], (*iter).addr[2], (*iter).addr[3],
                (*iter).addr[4], (*iter).addr[5], (*iter).addr[6], (*iter).addr[7],
                (*iter).addr[8], (*iter).addr[9], (*iter).addr[10], (*iter).addr[11],
                (*iter).addr[12], (*iter).addr[13], (*iter).addr[14], (*iter).addr[15]);
        }
    }

    if (inc_dns && !src->name_list.empty()) {
        dst->name_list = src->name_list;
        DHCP6_WALK_FWD_LIST(dst->name_list, iter) {
            T_N("NLST: %s", &(*iter)[0]);
        }
    }

    if (inc_adr && !src->adrs_pool.empty()) {
        dst->adrs_pool = src->adrs_pool;
        DHCP6_WALK_FWD_LIST(dst->adrs_pool, iter) {
            T_N("ADDR: %2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x:%2x%2x",
                (*iter).address.addr[0], (*iter).address.addr[1], (*iter).address.addr[2], (*iter).address.addr[3],
                (*iter).address.addr[4], (*iter).address.addr[5], (*iter).address.addr[6], (*iter).address.addr[7],
                (*iter).address.addr[8], (*iter).address.addr[9], (*iter).address.addr[10], (*iter).address.addr[11],
                (*iter).address.addr[12], (*iter).address.addr[13], (*iter).address.addr[14], (*iter).address.addr[15]);
        }
    }

    return TRUE;
}

static BOOL CLR_server_record(BOOL destroy, ServerRecord *const srv)
{
    if (!srv) {
        return FALSE;
    }

    vtss_clear(*srv);
    return TRUE;
}

static BOOL COMMIT_server_record(Interface *const ifdx, BOOL inc_adr, BOOL inc_dns)
{
    u8              val, flag, prf;
    ServerRecord    *candidate;

    if (!ifdx) {
        return FALSE;
    }

    flag = 0;   /* index */
    candidate = &ifdx->srvrs[0];
    val = DHCP6_SERVER_RECORD_CNT;
    for (; val; --val) {
        if (DHCP6_ADRS_EQUAL(&ifdx->srvrs[val].addrs, &candidate->addrs)) {
            flag = val;
            break;
        }
    }

    for (val = 1; flag == 0 && val <= DHCP6_SERVER_RECORD_CNT; ++val) {
        if (DHCP6_ADRS_ISZERO(&ifdx->srvrs[val].addrs)) {
            flag = val;
            break;
        }
    }

    /* find the least preference and replace it, if no more room */
    if (!flag) {
        u8  ref = DHCP6_SERVER_RECORD_PREF_GET(candidate);

        prf = ref;
        for (val = 1; ref && val <= DHCP6_SERVER_RECORD_CNT; ++val) {
            if (ref > DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[val]) &&
                prf > DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[val])) {
                prf = DHCP6_SERVER_RECORD_PREF_GET(&ifdx->srvrs[val]);
                flag = val;
            }
        }
    }

    T_D("%sFound ServerRecord(%u) to commit", flag ? "" : "Not", flag);
    if (flag) {
        if (!CPY_server_record(&ifdx->srvrs[flag], candidate, inc_adr, inc_dns)) {
            return FALSE;
        }

        if (!DETERMINE_active_server_record(ifdx)) {
            return FALSE;
        }

        return CLR_server_record(FALSE, candidate);
    } else {
        return FALSE;
    }
}

static BOOL VALIDATE_server_record(Interface *const ifdx, BOOL *adr_vld, BOOL *dns_chg)
{
    DeviceDuid      *a, *b;

    if (!ifdx || !adr_vld || !dns_chg) {
        return FALSE;
    }

    if (ifdx->rapid_commit &&
        ifdx->active_msg == DHCP6SOLICIT &&
        !ifdx->srvrs[0].rapid_commit) {
        return FALSE;
    }

    a = &ifdx->srvrs[0].duid;
    if (ifdx->server) {
        b = &ifdx->server->duid;
        if (!duid_equal(FALSE, a, FALSE, b)) {
            return FALSE;
        }
    } else {
        /* do nothing?! */
    }

    if (ifdx->active_msg == DHCP6CONFIRM) {
        if (ifdx->srvrs[0].adrs_pool.empty()) {
            *adr_vld = FALSE;
        } else {
            *adr_vld = TRUE;
        }
        *dns_chg = FALSE;
    } else {
        if (ifdx->active_msg == DHCP6DECLINE) {
            *dns_chg = FALSE;
        } else {
            *dns_chg = TRUE;
        }
    }

    return TRUE;
}

struct ControlServerRecord {
    mesa_ipv6_t     *sip;
    mesa_ipv6_t     *uip;
    u32             *xidx;
    AddrInfo        *addr;
    mesa_ipv6_t     *nsrv;
    DeviceDuid      *duid;
    BOOL            duid_nwo;
    BOOL            *rpcm;
    u8              *pref;
    char            *nlst;
};

static BOOL SET_server_record(const ControlServerRecord *const src, ServerRecord *const srv)
{
    BOOL    fnd;

    if (!src || !srv) {
        return FALSE;
    }

    if (src->sip) {
        DHCP6_ADRS_CPY(&srv->addrs, src->sip);
    }
    if (src->uip) {
        DHCP6_ADRS_CPY(&srv->ucast, src->uip);
        srv->has_unicast = TRUE;
    }
    if (src->xidx) {
        DHCP6_SERVER_RECORD_XIDX_SET(srv, *(src->xidx));
    }
    if (src->addr) {
        if (srv->adrs_pool.empty()) {
            srv->adrs_pool.insert_after(srv->adrs_pool.before_begin(), *(src->addr));
        } else {
            auto    prev = srv->adrs_pool.before_begin();

            fnd = FALSE;
            for (auto iter = srv->adrs_pool.begin(); iter != srv->adrs_pool.end(); prev = iter, ++iter) {
                if (DHCP6_ADRS_EQUAL(&(*iter).address, &src->addr->address)) {
                    memcpy(&(*iter), src->addr, sizeof(AddrInfo));
                    fnd = TRUE;
                    break;
                }
            }
            if (!fnd) {  /* prev presents last element */
                srv->adrs_pool.insert_after(prev, *(src->addr));
            }
        }
    }
    if (src->nsrv) {
        if (srv->name_server.empty()) {
            srv->name_server.insert_after(srv->name_server.before_begin(), *(src->nsrv));
        } else {
            auto    prev = srv->name_server.before_begin();

            fnd = FALSE;
            for (auto iter = srv->name_server.begin(); iter != srv->name_server.end(); prev = iter, ++iter) {
                if (DHCP6_ADRS_EQUAL(&(*iter), src->nsrv)) {
                    fnd = TRUE;
                    break;
                }
            }
            if (!fnd) {  /* prev presents last element */
                srv->name_server.insert_after(prev, *(src->nsrv));
            }
        }
    }
    if (src->duid && !duid_assign(FALSE, &srv->duid, src->duid_nwo, src->duid)) {
        return FALSE;
    }
    if (src->rpcm) {
        srv->rapid_commit = TRUE;
    }
    if (src->pref) {
        DHCP6_SERVER_RECORD_PREF_SET(srv, *(src->pref));
    }
    if (src->nlst) {
        if (srv->name_list.empty()) {
            srv->name_list.insert_after(srv->name_list.before_begin(), src->nlst);
        } else {
            auto    prev = srv->name_list.before_begin();

            fnd = FALSE;
            for (auto iter = srv->name_list.begin(); iter != srv->name_list.end(); prev = iter, ++iter) {
                if (!(*iter).compare(src->nlst)) {
                    fnd = TRUE;
                    break;
                }
            }
            if (!fnd) {  /* prev presents last element */
                srv->name_list.insert_after(prev, src->nlst);
            }
        }
    }

    return TRUE;
}

#define DHCP6_INTF_SERVER_RECORD_CLR(x, y)      do {    \
    u8  _cnt = DHCP6_SERVER_RECORD_CNT + 1;             \
    for (; _cnt; --_cnt) {                              \
        CLR_server_record((y), &(x)->srvrs[_cnt - 1]);  \
    }                                                   \
} while (0)

#define DHCP6_INTF_SERVER_RECORD_CPY(x, y)      do {    \
    u8              _snt;                               \
    ServerRecord    *_a;                                \
    ServerRecord    *_b;                                \
    DHCP6_INTF_SERVER_RECORD_CLR((x), FALSE);           \
    _snt = DHCP6_SERVER_RECORD_CNT + 1;                 \
    for (; _snt; --_snt) {                              \
        _b = (ServerRecord *)&((y)->srvrs[_snt - 1]);   \
        if (DHCP6_ADRS_ISZERO(&_b->addrs)) continue;      \
        _a = (ServerRecord *)&((x)->srvrs[_snt - 1]);   \
        if (!CPY_server_record(_a, _b, TRUE, TRUE)) {}; \
    }                                                   \
} while (0)

#define DHCP6_RXMT_INIT_MSG(x, y)               do {    \
    AddrInfo    *pa;                                    \
    Rxmit       *pb;                                    \
    if (!(x)) break;                                    \
    (x)->active_msg = (y);                              \
    (x)->xidx = 0;                                      \
    (x)->last_xmt = 0;                                  \
    (x)->xmt_cntr = 0;                                  \
    (x)->init_xmt = 0;                                  \
    pa = &((x)->addrs);                                 \
    pb = &((x)->rxmit);                                 \
    memset(pb, 0x0, sizeof(Rxmit));                     \
    switch ( (y) ) { /* RFC-3315: 5.5 */                \
    case DHCP6SOLICIT:                                  \
        pb->max_delay = DHCPV6_SOL_MAX_DELAY;           \
        pb->init_rxmit_time = DHCPV6_SOL_TIMEOUT;       \
        pb->max_rxmit_time = DHCPV6_SOL_MAX_RT;         \
        break;                                          \
    case DHCP6REQUEST:                                  \
        pb->init_rxmit_time = DHCPV6_REQ_TIMEOUT;       \
        pb->max_rxmit_time = DHCPV6_REQ_MAX_RT;         \
        pb->max_rxmit_count = DHCPV6_REQ_MAX_RC;        \
        break;                                          \
    case DHCP6CONFIRM:                                  \
        pb->max_delay = DHCPV6_CNF_MAX_DELAY;           \
        pb->init_rxmit_time = DHCPV6_CNF_TIMEOUT;       \
        pb->max_rxmit_time = DHCPV6_CNF_MAX_RT;         \
        DHCP6_RXMT_MRD_SET(pb, DHCPV6_CNF_MAX_RD);      \
        break;                                          \
    case DHCP6RENEW:                                    \
        pb->init_rxmit_time = DHCPV6_REN_TIMEOUT;       \
        pb->max_rxmit_time = DHCPV6_REN_MAX_RT;         \
        DHCP6_RXMT_MRD_SET(pb, pa->t2 - 1);             \
        break;                                          \
    case DHCP6REBIND:                                   \
        pb->init_rxmit_time = DHCPV6_REB_TIMEOUT;       \
        pb->max_rxmit_time = DHCPV6_REB_MAX_RT;         \
        DHCP6_RXMT_MRD_SET(pb, pa->valid_lifetime);     \
        break;                                          \
    case DHCP6RELEASE:                                  \
        pb->init_rxmit_time = DHCPV6_REL_TIMEOUT;       \
        pb->max_rxmit_count = DHCPV6_REL_MAX_RC;        \
        break;                                          \
    case DHCP6DECLINE:                                  \
        pb->init_rxmit_time = DHCPV6_DEC_TIMEOUT;       \
        pb->max_rxmit_count = DHCPV6_DEC_MAX_RC;        \
        break;                                          \
    case DHCP6INFORMATION_REQUEST:                      \
        pb->max_delay = DHCPV6_INF_MAX_DELAY;           \
        pb->init_rxmit_time = DHCPV6_INF_TIMEOUT;       \
        pb->max_rxmit_time = DHCPV6_INF_MAX_RT;         \
        break;                                          \
    case DHCP6RECONFIGURE:                              \
        pb->init_rxmit_time = DHCPV6_REC_TIMEOUT;       \
        pb->max_rxmit_count = DHCPV6_REC_MAX_RC;        \
    case DHCP6_MSG_INIT:                                \
        DHCP6_INTF_SERVER_RECORD_CLR(x, FALSE);         \
        (x)->server = NULL;                             \
        break;                                          \
    case DHCP6ADVERTISE:                                \
    case DHCP6REPLY:                                    \
    default:                                            \
        break;                                          \
    }                                                   \
} while (0)

#define OPTI_IAADDR_LEN         (sizeof(OptIaAddr) - sizeof(OptIaAddr::options))
#define OPTI_IADATA_LEN         (OPTI_IAADDR_LEN - sizeof(OptCommon))
#define CALC_IAADDR_LEN(x, y, z)   do {                 \
    if (((x) -= (y)) < OPTI_IAADDR_LEN) {               \
        T_D("Invalid OptIaAddr Length(%u)", (x));       \
        VTSS_FREE((z));                                 \
        return VTSS_RC_ERROR;                           \
    }                                                   \
} while (0)

#define NEXT_IAADDR_OPT(x, y, z)   do {                 \
    if (DHCP6_OPT_LEN_GET((x)) >= (y)) {                \
        (x) = NULL;                                     \
    } else {                                            \
        u8  *_p = (u8 *)(x);                            \
        CALC_IAADDR_LEN((y),DHCP6_OPT_LEN_GET((x)),(z));\
        _p += DHCP6_OPT_LEN_GET((x));                   \
        (x) = (OptIaAddr *)_p;                          \
    }                                                   \
} while (0)

mesa_rc do_rx_solicit(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

static mesa_rc RX_advertise(const mesa_ipv6_t *const sip, Interface *const ifdx, u32 xidx, u16 code, u16 length, const u8 *const data)
{
    if (ifdx) {
        u8  val;
        u16 flag;

        if (ifdx->xidx != xidx) {
            T_D("XID mismatched: H(%x) != S(%x)", ifdx->xidx, xidx);
            return VTSS_RC_ERROR;
        }

        switch ( code ) {
        case OPT_CLIENTID:
            if (!data) {
                return VTSS_RC_ERROR;
            } else {
                DeviceDuid  *a, *b;

                a = &ifdx->duid;
                b = (DeviceDuid *)data;
                if (!duid_equal(FALSE, a, TRUE, b)) {
                    return VTSS_RC_ERROR;
                }
            }
            break;
        case OPT_PREFERENCE:
        case OPT_SERVERID:
        case OPT_RAPID_COMMIT:
        case OPT_IA_NA:
        case OPT_IA_TA:
        case OPT_DNS_SERVERS:
        case OPT_DOMAIN_LIST:
        case OPT_UNICAST:
            if (!sip) {
                return VTSS_RC_ERROR;
            } else {
                mesa_ipv6_t         srv_adr, alt_adr;
                ControlServerRecord ctrl_srv;
                ServerRecord        *c;

                if (code != OPT_RAPID_COMMIT && !data) {
                    return VTSS_RC_ERROR;
                }

                c = &ifdx->srvrs[0];
                memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                DHCP6_ADRS_CPY(&srv_adr, sip);
                ctrl_srv.sip = &srv_adr;
                ctrl_srv.xidx = &ifdx->xidx;
                if (code == OPT_RAPID_COMMIT) {
                    val = 1;
                    ctrl_srv.rpcm = &val;
                } else if (code == OPT_SERVERID) {
                    DeviceDuid  srv_duid;

                    if (duid_assign(FALSE, &srv_duid, TRUE, (DeviceDuid *)data)) {
                        ctrl_srv.duid = &srv_duid;
                    }
                } else if (code == OPT_PREFERENCE) {
                    val = *data;
                    ctrl_srv.pref = &val;
                } else if (code == OPT_DOMAIN_LIST) {
                    char domain_name[255];

                    flag = 0;
                    while (flag < length && DHCP6_build_domain_name(&flag, (char *)data, domain_name)) {
                        T_D("NLIST:%s/OFST:%u", domain_name, flag);
                        ctrl_srv.nlst = domain_name;
                        if (!SET_server_record(&ctrl_srv, c)) {
                            return VTSS_RC_ERROR;
                        }
                    }
                    memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                } else if (code == OPT_UNICAST) {
                    DHCP6_ADRS_CPY(&alt_adr, (mesa_ipv6_t *)data);
                    ctrl_srv.uip = &alt_adr;
                } else if (code == OPT_DNS_SERVERS) {
                    flag = length / sizeof(mesa_ipv6_t);
                    ctrl_srv.nsrv = &alt_adr;
                    for (val = 0; val < (u8)(flag & 0xFF); ++val) {
                        T_D_HEX((data + (val * sizeof(mesa_ipv6_t))), sizeof(mesa_ipv6_t));
                        DHCP6_ADRS_CPY(&alt_adr, (mesa_ipv6_t *)(data + (val * sizeof(mesa_ipv6_t))));
                        if (!SET_server_record(&ctrl_srv, c)) {
                            return VTSS_RC_ERROR;
                        }
                    }
                    memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                } else if (code == OPT_IA_NA || code == OPT_IA_TA) {
                    u32             *iap, *iax;
                    AddrInfo        iaadr;
                    OptIaAddr       *adr;
                    OptStatusCode   *ss;

                    if (VTSS_CALLOC_CAST(iap, 1, sizeof(int) * ((length + 3) / sizeof(int))) == NULL) {
                        T_D("Cannot handle IAX: Memory allocation failure");
                        return VTSS_RC_ERROR;
                    } else {
                        memcpy((u8 *)iap, data, length);
                    }
                    iax = iap;
                    adr = NULL;

                    flag = length;
                    if (ifdx->iaid != ntohl(*iax)) {
                        T_D("IAID Mismatch: %x != %x", ifdx->iaid, ntohl(*iax));
                        VTSS_FREE(iap);
                        return VTSS_RC_ERROR;
                    }

                    iax++;
                    if (code == OPT_IA_NA) {
                        iaadr.type = IA_TYPE_NA;
                        CALC_IAADDR_LEN(flag, sizeof(*iax), iap);
                        iaadr.t1 = ntohl(*iax++);
                        CALC_IAADDR_LEN(flag, sizeof(*iax), iap);
                        iaadr.t2 = ntohl(*iax++);
                        CALC_IAADDR_LEN(flag, sizeof(*iax), iap);
                    }
                    if (code == OPT_IA_TA) {
                        iaadr.type = IA_TYPE_TA;
                        CALC_IAADDR_LEN(flag, sizeof(*iax), iap);
                        iaadr.t1 = iaadr.t2 = 0;
                    }
                    /*
                        If a client receives an IA_NA with T1 greater than T2, and both T1
                        and T2 are greater than 0, the client discards the IA_NA option and
                        processes the remainder of the message as though the server had not
                        included the invalid IA_NA option.
                    */
                    if (code == OPT_IA_NA && iaadr.t1 && iaadr.t2) {
                        if (iaadr.t1 > iaadr.t2) {
                            T_D("T1(" VPRI64u") > T2(" VPRI64u")", iaadr.t1, iaadr.t2);
                            VTSS_FREE(iap);
                            return VTSS_RC_OK;
                        }
                    }

                    adr = (OptIaAddr *)iax;
                    ctrl_srv.addr = &iaadr;
                    while (adr && flag) {
                        T_D("IAADDR OP:%x LEN:%zu/%u", DHCP6_OPT_CODE_GET(adr), DHCP6_OPT_LEN_GET(adr), flag);
                        T_D_HEX((u8 *)adr, DHCP6_OPT_LEN_GET(adr));
                        if (DHCP6_OPT_CODE_GET(adr) != OPT_IAADDR) {
                            NEXT_IAADDR_OPT(adr, flag, iap);
                            continue;
                        }
                        if (DHCP6_OPT_DATA_LEN_GET(adr) > OPTI_IADATA_LEN) {
                            ss = (OptStatusCode *)&adr->options;
                            if (DHCP6_OPT_CODE_GET(ss) == OPT_STATUS_CODE &&
                                ntohs(ss->status_code) != STATUS_SUCCESS) {
                                NEXT_IAADDR_OPT(adr, flag, iap);
                                continue;
                            }
                        }

                        iaadr.preferred_lifetime = ntohl(adr->preferred_lifetime);
                        iaadr.valid_lifetime = ntohl(adr->valid_lifetime);
                        if (!VTSS_DHCP6_INFINITE_LT(iaadr.valid_lifetime)) {
                            /*
                                RFC-3315: 22.6
                                A client discards any addresses for which the preferred
                                lifetime is greater than the valid lifetime.
                            */
                            if (iaadr.valid_lifetime < iaadr.preferred_lifetime) {
                                NEXT_IAADDR_OPT(adr, flag, iap);
                                continue;
                            }
                        }

                        iaadr.prefix_length = 128;
                        /* adr->options may present OptStatusCode */
                        DHCP6_ADRS_CPY(&iaadr.address, &adr->address);
                        if (!SET_server_record(&ctrl_srv, c)) {
                            VTSS_FREE(iap);
                            return VTSS_RC_ERROR;
                        }
                        NEXT_IAADDR_OPT(adr, flag, iap);
                    }
                    VTSS_FREE(iap);
                    memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                }

                if (!SET_server_record(&ctrl_srv, c) ||
                    interface_set(ifdx->ifidx, ifdx) != VTSS_RC_OK) {
                    return VTSS_RC_ERROR;
                }
            }
            break;
        default:    /* Skip unknown options */
            T_D("Skip OptionCode: %u", code);
            break;
        }

        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

mesa_rc do_rx_advertise(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    Interface   *ifdx;
    mesa_rc     rc;

    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) == NULL) {
        T_D("MemAlloc Failure");
        return VTSS_RC_ERROR;
    }

    if ((rc = interface_get(ifx, ifdx)) == VTSS_RC_OK) {
        if (ifdx->active_msg != DHCP6SOLICIT) {
            T_D("ActiveMessage is %u", ifdx->active_msg);
            rc = VTSS_RC_ERROR;
        } else {
            if (commit) {
                T_D("COMMIT DHCP6S RECORD(" VPRI64u")", vtss_current_time());
                /* determine active server */
                if (COMMIT_server_record(ifdx, TRUE, TRUE)) {
                    BOOL    do_transit = (ifdx->rxmit.rxmit_timeout <= vtss_current_time());

                    if (!do_transit && ifdx->active_msg == DHCP6SOLICIT) {
                        /*
                            RFC-3315: 17.1.2
                            If the client does not receive any Advertise messages before the
                            first RT has elapsed, it begins the retransmission. The client
                            terminates the retransmission process as soon as it receives any
                            Advertise message, and the client acts on the received Advertise
                            message without waiting for any additional Advertise messages.
                        */
                        if (ifdx->xmt_cntr > 1) {
                            do_transit = TRUE;
                        }
                    }

                    T_D("%sTRANSIT",  do_transit? "" : "No");
                    if (do_transit) {
                        DHCP6_RXMT_INIT_MSG(ifdx, DHCP6REQUEST);
                        DHCP6_TX_XIDX_GENERATE(ifdx);
                        DHCP6_RXMT_RT_ASSIGN(&ifdx->rxmit, 0);
                        ifdx->last_xmt = ifdx->rxmit.rxmit_timeout;
                    }

                    rc = interface_set(ifx, ifdx);
                } else {
                    T_D("Commit SrvRcd Failure");
                    rc = VTSS_RC_ERROR;
                }
            } else {
                rc = RX_advertise(sip, ifdx, xidx, code, length, data);
            }
        }
    }

    DHCP6_INTF_MEM_FREE(ifdx);
    return rc;
}

mesa_rc do_rx_request(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

mesa_rc do_rx_confirm(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

mesa_rc do_rx_renew(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

mesa_rc do_rx_rebind(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

static mesa_rc RX_reply(const mesa_ipv6_t *const sip, Interface *const ifdx, u32 xidx, u16 code, u16 length, const u8 *const data)
{
    if (ifdx) {
        u8  val;
        u16 flag;

        if (ifdx->xidx != xidx) {
            T_D("XID mismatched: H(%x) != S(%x)", ifdx->xidx, xidx);
            return VTSS_RC_ERROR;
        }

        switch ( code ) {
        case OPT_CLIENTID:
            if (!data) {
                return VTSS_RC_ERROR;
            } else {
                DeviceDuid  *a, *b;

                a = &ifdx->duid;
                b = (DeviceDuid *)data;
                if (!duid_equal(FALSE, a, TRUE, b)) {
                    return VTSS_RC_ERROR;
                }
            }
            break;
        case OPT_PREFERENCE:
        case OPT_SERVERID:
        case OPT_RAPID_COMMIT:
        case OPT_IA_NA:
        case OPT_IA_TA:
        case OPT_DNS_SERVERS:
        case OPT_DOMAIN_LIST:
        case OPT_UNICAST:
        case OPT_STATUS_CODE:
            if (!sip) {
                return VTSS_RC_ERROR;
            } else {
                mesa_ipv6_t         srv_adr, alt_adr;
                ControlServerRecord ctrl_srv;
                ServerRecord        *c;
                BOOL                intr_rc;

                if (code != OPT_RAPID_COMMIT && !data) {
                    return VTSS_RC_ERROR;
                }

                intr_rc = FALSE;
                c = &ifdx->srvrs[0];
                memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                DHCP6_ADRS_CPY(&srv_adr, sip);
                ctrl_srv.sip = &srv_adr;
                ctrl_srv.xidx = &ifdx->xidx;
                if (code == OPT_RAPID_COMMIT) {
                    val = 1;
                    ctrl_srv.rpcm = &val;
                } else if (code == OPT_STATUS_CODE) {
                    /* RFC-3315: 18.1.8 */
                    if (ifdx->active_msg != DHCP6RELEASE && ifdx->active_msg != DHCP6DECLINE) {
                        u16 *sc = (u16 *)data;

                        flag = ntohs(*sc);
                        if (flag == STATUS_UNSPEC_FAIL) {
                        } else if (flag == STATUS_USE_MULTICAST) {
                        } else if (flag == STATUS_NOT_ON_LINK) {
                            if (ifdx->active_msg == DHCP6CONFIRM ||
                                ifdx->active_msg == DHCP6REQUEST) {
                                DHCP6_RXMT_INIT_MSG(ifdx, DHCP6SOLICIT);
                                DHCP6_TX_XIDX_GENERATE(ifdx);
                                intr_rc = TRUE;
                            }
                        }
                    }
                } else if (code == OPT_SERVERID) {
                    DeviceDuid  srv_duid;

                    if (duid_assign(FALSE, &srv_duid, TRUE, (DeviceDuid *)data)) {
                        ctrl_srv.duid = &srv_duid;
                    }
                } else if (code == OPT_PREFERENCE) {
                    val = *data;
                    ctrl_srv.pref = &val;
                } else if (code == OPT_DOMAIN_LIST) {
                    char domain_name[255];

                    flag = 0;
                    while (flag < length && DHCP6_build_domain_name(&flag, (char *)data, domain_name)) {
                        T_D("NLIST:%s/OFST:%u", domain_name, flag);
                        ctrl_srv.nlst = domain_name;
                        if (!SET_server_record(&ctrl_srv, c)) {
                            return VTSS_RC_ERROR;
                        }
                    }
                    memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                } else if (code == OPT_UNICAST) {
                    DHCP6_ADRS_CPY(&alt_adr, (mesa_ipv6_t *)data);
                    ctrl_srv.uip = &alt_adr;
                } else if (code == OPT_DNS_SERVERS) {
                    flag = length / sizeof(mesa_ipv6_t);
                    ctrl_srv.nsrv = &alt_adr;
                    for (val = 0; val < (u8)(flag & 0xFF); ++val) {
                        T_D_HEX((data + (val * sizeof(mesa_ipv6_t))), sizeof(mesa_ipv6_t));
                        DHCP6_ADRS_CPY(&alt_adr, (mesa_ipv6_t *)(data + (val * sizeof(mesa_ipv6_t))));
                        if (!SET_server_record(&ctrl_srv, c)) {
                            return VTSS_RC_ERROR;
                        }
                    }
                    memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                } else if (code == OPT_IA_NA || code == OPT_IA_TA) {
                    u32             *iap, *iax;
                    AddrInfo        iaadr;
                    OptIaAddr       *adr;
                    OptStatusCode   *ss;
                    u16             sck;
                    BOOL            by_pas, do_sck;

                    if (VTSS_CALLOC_CAST(iap, 1, sizeof(int) * ((length + 3) / sizeof(int))) == NULL) {
                        T_D("Cannot handle IAX: Memory allocation failure");
                        return VTSS_RC_ERROR;
                    } else {
                        memcpy((u8 *)iap, data, length);
                    }
                    iax = iap;
                    adr = NULL;
                    by_pas = (ifdx->active_msg == DHCP6DECLINE || ifdx->active_msg == DHCP6RELEASE);
                    do_sck = (ifdx->active_msg == DHCP6RENEW || ifdx->active_msg == DHCP6REBIND);

                    flag = length;
                    if (ifdx->iaid != ntohl(*iax)) {
                        T_D("IAID Mismatch: %x != %x", ifdx->iaid, ntohl(*iax));
                        VTSS_FREE(iap);
                        return VTSS_RC_ERROR;
                    }

                    iax++;
                    if (code == OPT_IA_NA) {
                        if (flag < sizeof(OptIaNa) - sizeof(OptCommon)) {
                            T_D("Invalid OptIaNa Length(%u)", flag);
                            VTSS_FREE(iap);
                            return VTSS_RC_ERROR;
                        }

                        iaadr.type = IA_TYPE_NA;
                        flag -= sizeof(*iax);
                        iaadr.t1 = ntohl(*iax++);
                        flag -= sizeof(*iax);
                        iaadr.t2 = ntohl(*iax++);
                        ss = NULL;
                        if (flag >= (sizeof(OptStatusCode) - sizeof(OptStatusCode::status_message))) {
                            ss = (OptStatusCode *)iax;
                        }
                        if (do_sck && ss) {
                            sck = DHCP6_OPT_CODE_GET(ss);
                            if (sck == OPT_STATUS_CODE) {
                                sck = ntohs(ss->status_code);
                                if (sck == STATUS_NO_BINDING) {
                                    DHCP6_RXMT_INIT_MSG(ifdx, DHCP6REQUEST);
                                    DHCP6_TX_XIDX_GENERATE(ifdx);
                                    if (interface_set(ifdx->ifidx, ifdx) != VTSS_RC_OK) {
                                        T_D("Failed to process STATUS_NO_BINDING");
                                    }
                                    VTSS_FREE(iap);
                                    return VTSS_RC_ERROR;
                                } else if (sck != STATUS_SUCCESS && sck != STATUS_USE_MULTICAST) {
                                    T_D("Invalid OptIaNa Option(%u)", sck);
                                    VTSS_FREE(iap);
                                    return VTSS_RC_ERROR;
                                } else {
                                    sck = DHCP6_OPT_LEN_GET(ss);
                                    if (flag > sck) {
                                        T_D("Process remainning OptIaAddr");
                                        flag -= sck;
                                        iax = (u32 *)((u8 *)ss + sck);
                                    } else if (flag == sck) {
                                        VTSS_FREE(iap);
                                        return VTSS_RC_OK;
                                    } else {
                                        T_D("Invalid OptIaNa Option(%u)", sck);
                                        VTSS_FREE(iap);
                                        return VTSS_RC_ERROR;
                                    }
                                }
                            } else if (sck == OPT_IAADDR) {
                                CALC_IAADDR_LEN(flag, sizeof(*iax), iap);
                            } else {
                                T_D("Invalid OptIaNa Option(%u)", sck);
                                VTSS_FREE(iap);
                                return VTSS_RC_ERROR;
                            }
                        } else {
                            if (do_sck) {
                                T_D("Invalid OptIaNa-Option Length(%u)", flag);
                                VTSS_FREE(iap);
                                return VTSS_RC_ERROR;
                            } else {
                                if (by_pas) {
                                    VTSS_FREE(iap);
                                    return VTSS_RC_OK;
                                } else {
                                    CALC_IAADDR_LEN(flag, sizeof(*iax), iap);
                                }
                            }
                        }
                    }
                    if (code == OPT_IA_TA) {
                        if (flag < sizeof(OptIaTa) - sizeof(OptCommon)) {
                            T_D("Invalid OptIaTa Length(%u)", flag);
                            VTSS_FREE(iap);
                            return VTSS_RC_ERROR;
                        }

                        iaadr.type = IA_TYPE_TA;
                        CALC_IAADDR_LEN(flag, sizeof(*iax), iap);
                        iaadr.t1 = iaadr.t2 = 0;
                    }
                    /*
                        If a client receives an IA_NA with T1 greater than T2, and both T1
                        and T2 are greater than 0, the client discards the IA_NA option and
                        processes the remainder of the message as though the server had not
                        included the invalid IA_NA option.
                    */
                    if (code == OPT_IA_NA && iaadr.t1 && iaadr.t2) {
                        if (iaadr.t1 > iaadr.t2) {
                            T_D("T1(" VPRI64u") > T2(" VPRI64u")", iaadr.t1, iaadr.t2);
                            VTSS_FREE(iap);
                            return VTSS_RC_OK;
                        }
                    }

                    adr = (OptIaAddr *)iax;
                    ctrl_srv.addr = &iaadr;
                    while (adr && flag) {
                        T_D("IAADDR OP:%x LEN:%zu/%u", DHCP6_OPT_CODE_GET(adr), DHCP6_OPT_LEN_GET(adr), flag);
                        T_D_HEX((u8 *)&adr->address, sizeof(mesa_ipv6_t));
                        if (DHCP6_OPT_CODE_GET(adr) != OPT_IAADDR) {
                            NEXT_IAADDR_OPT(adr, flag, iap);
                            continue;
                        }
                        if (DHCP6_OPT_DATA_LEN_GET(adr) > OPTI_IADATA_LEN) {
                            ss = (OptStatusCode *)&adr->options;
                            if (DHCP6_OPT_CODE_GET(ss) == OPT_STATUS_CODE) {
                                sck = ntohs(ss->status_code);
                                if (sck != STATUS_SUCCESS) {
                                    if (sck == STATUS_NO_ADDRS_AVAIL) {
                                        DHCP6_RXMT_INIT_MSG(ifdx, DHCP6SOLICIT);
                                        DHCP6_TX_XIDX_GENERATE(ifdx);
                                        intr_rc = TRUE;
                                        break;
                                    }

                                    if (sck == STATUS_NO_BINDING) {
                                        if (do_sck) {
                                            DHCP6_RXMT_INIT_MSG(ifdx, DHCP6REQUEST);
                                            DHCP6_TX_XIDX_GENERATE(ifdx);
                                            intr_rc = TRUE;
                                            break;
                                        }
                                    }

                                    NEXT_IAADDR_OPT(adr, flag, iap);
                                    continue;
                                }
                            }
                        }

                        iaadr.preferred_lifetime = ntohl(adr->preferred_lifetime);
                        iaadr.valid_lifetime = ntohl(adr->valid_lifetime);
                        if (!VTSS_DHCP6_INFINITE_LT(iaadr.valid_lifetime)) {
                            /*
                                RFC-3315: 18.1.8
                                Discard any addresses from the IA, as recorded by the client, that
                                have a valid lifetime of 0 in the IA Address option.
                                RFC-3315: 22.6
                                A client discards any addresses for which the preferred
                                lifetime is greater than the valid lifetime.
                            */
                            if (iaadr.valid_lifetime == 0 ||
                                iaadr.valid_lifetime < iaadr.preferred_lifetime) {
                                iaadr.type = IA_TYPE_INVALID;
                            }
                        }
                        if (iaadr.type != IA_TYPE_INVALID && iaadr.t1 == 0 && iaadr.t2 == 0) {
                            /*
                             * RFC 3315, section 22.4: If the time at which the addresses in an IA_NA
                             * are to be renewed is to be left to the discretion of the client,
                             * the server sets T1 and T2 to 0.
                             */
                            if (VTSS_DHCP6_INFINITE_LT(iaadr.preferred_lifetime)) {
                                /*
                                 * If the "shortest" preferred lifetime is 0xffffffff ("infinity"),
                                 * the recommended T1 and T2 values are also 0xffffffff.
                                 */
                                iaadr.t1 = VTSS_DHCP6_LIFETIME_INFINITY;
                                iaadr.t2 = VTSS_DHCP6_LIFETIME_INFINITY;

                            } else {
                                /*
                                 * Set T1 and T2 to .5 and .8 times the pref. lifetime respectively.
                                 * Do floating point calculation for the sake of precision.
                                 */
                                iaadr.t1 = (vtss_tick_count_t)(iaadr.preferred_lifetime / 2.0);
                                iaadr.t2 = (vtss_tick_count_t)(8.0 * iaadr.preferred_lifetime / 10.0);
                            }

                            T_D("T1: %u secs, T2: %u secs (calculated)", iaadr.t1, iaadr.t2);
                        }

                        iaadr.prefix_length = 128;
                        /* adr->options may present OptStatusCode */
                        DHCP6_ADRS_CPY(&iaadr.address, &adr->address);
                        if (!SET_server_record(&ctrl_srv, c)) {
                            VTSS_FREE(iap);
                            return VTSS_RC_ERROR;
                        }
                        NEXT_IAADDR_OPT(adr, flag, iap);
                    }
                    VTSS_FREE(iap);
                    memset(&ctrl_srv, 0x0, sizeof(ControlServerRecord));
                }

                if (!SET_server_record(&ctrl_srv, c) ||
                    interface_set(ifdx->ifidx, ifdx) != VTSS_RC_OK ||
                    intr_rc) {
                    return VTSS_RC_ERROR;
                }
            }
            break;
        default:    /* Skip unknown options */
            T_D("Skip OptionCode: %u", code);
            break;
        }

        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

mesa_rc do_rx_reply(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    Interface   *ifdx;
    mesa_rc     rc;

    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx) == NULL) {
        return VTSS_RC_ERROR;
    }

    if ((rc = interface_get(ifx, ifdx)) == VTSS_RC_OK) {
        if (ifdx->active_msg != DHCP6SOLICIT &&
            ifdx->active_msg != DHCP6REQUEST &&
            ifdx->active_msg != DHCP6CONFIRM &&
            ifdx->active_msg != DHCP6RENEW &&
            ifdx->active_msg != DHCP6REBIND &&
            ifdx->active_msg != DHCP6RELEASE &&
            ifdx->active_msg != DHCP6DECLINE &&
            ifdx->active_msg != DHCP6INFORMATION_REQUEST) {
            rc = VTSS_RC_ERROR;
        } else {
            if (commit) {
                BOOL    inc_adr = TRUE, inc_dns = TRUE;

                T_D("COMMIT DHCP6S RECORD");
                if (VALIDATE_server_record(ifdx, &inc_adr, &inc_dns) &&
                    COMMIT_server_record(ifdx, inc_adr, inc_dns)) {
                    if (ifdx->active_msg == DHCP6DECLINE || ifdx->active_msg == DHCP6RELEASE) {
                        DHCP6_RXMT_INIT_MSG(ifdx, DHCP6_MSG_INIT);
                        rc = interface_set(ifdx->ifidx, ifdx);
                        T_D("COMMIT bypass(%d)", rc);
                    } else {
                        BOOL        do_sck = (ifdx->active_msg == DHCP6RENEW || ifdx->active_msg == DHCP6REBIND);
                        mesa_ipv6_t adrs_ask;

                        memset(&adrs_ask, 0x0, sizeof(mesa_ipv6_t));
                        if (do_sck && !DHCP6_ADRS_ISZERO(&ifdx->addrs.address)) {
                            T_D("BackupAdrsAsk");
                            DHCP6_ADRS_CPY(&adrs_ask, &ifdx->addrs.address);
                        }
                        if (APPLY_server_record(ifdx)) {
                            rc = VTSS_RC_OK;
                            if (ifdx->active_msg != DHCP6INFORMATION_REQUEST) {
                                if (do_sck) {
                                    T_D("CheckAdrsAsk");
                                    if (!DHCP6_ADRS_ISZERO(&adrs_ask) &&
                                        !DHCP6_ADRS_EQUAL(&adrs_ask, &ifdx->addrs.address)) {
                                        T_D("NotExpectedAddress");
                                        rc = VTSS_RC_ERROR;
                                    }
                                    T_D("CheckIfdxAddrType(%d)", ifdx->addrs.type);
                                    if (ifdx->addrs.type == IA_TYPE_INVALID) {
                                        T_D("NotValidAddress");
                                        rc = VTSS_RC_ERROR;
                                    }
                                }

                                if (rc == VTSS_RC_OK) {
                                    DHCPV6C_CRIT_ENTER();
                                    rc = DHCP6_ipstk_address_add(ifdx);
                                    DHCPV6C_CRIT_EXIT();
                                }
                            }
#ifdef VTSS_SW_OPTION_DNS
                            if (rc == VTSS_RC_OK) {
                                vtss_dns_signal();
                            }
#endif /* VTSS_SW_OPTION_DNS */
                            T_D("APPLY done(%d)", rc);
                        } else {
                            rc = VTSS_RC_ERROR;
                            T_D("APPLY error");
                        }

                        if (rc == VTSS_RC_OK) {
                            DHCP6_RXMT_INIT_MSG(ifdx, DHCP6RENEW);
                            DHCP6_TX_XIDX_GENERATE(ifdx);
                            DHCP6_RXMT_RT_ASSIGN(&ifdx->rxmit, DHCP6_RXMT_SEC_TO_TICK(ifdx->addrs.t1));
                            ifdx->last_xmt = vtss_current_time();
                            rc = interface_set(ifdx->ifidx, ifdx);
                        }
                        T_D("COMMIT done(%d)", rc);
                    }
                } else {
                    rc = VTSS_RC_ERROR;
                    T_D("COMMIT error");
                }
            } else {
                rc = RX_reply(sip, ifdx, xidx, code, length, data);
            }
        }
    }

    DHCP6_INTF_MEM_FREE(ifdx);
    return rc;
}

mesa_rc do_rx_release(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

mesa_rc do_rx_decline(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

mesa_rc do_rx_reconfigure(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

#if DHCPV6_DO_AUTHENTICATION
    return VTSS_RC_OK;
#else
    return VTSS_RC_ERROR;
#endif /* DHCPV6_DO_AUTHENTICATION */
}

mesa_rc do_rx_information_request(
    BOOL                commit,
    const mesa_ipv6_t   *const sip,
    mesa_vid_t          ifx,
    u32                 xidx,
    u16                 code,
    u16                 length,
    const u8            *const data
)
{
    T_D("%s->Vid:%u/Xid:0x%x/Code:%u/Len:%u", commit ? "CMT" : "CHK", ifx, xidx, code, length);
    if (!commit) {
        T_D_HEX(data, length);
    }

    return VTSS_RC_ERROR;
}

#undef NEXT_IAADDR_OPT
#undef CALC_IAADDR_LEN
#undef OPTI_IADATA_LEN
#undef OPTI_IAADDR_LEN

mesa_rc do_tx_migrate_counter(Interface *const intf)
{
    Interface   *ifdx;
    mesa_rc     rc = VTSS_RC_ERROR;

    if (!intf) {
        return rc;
    }

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty()) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != intf->ifidx) {
            continue;
        }

        memcpy(&intf->cntrs, &ifdx->cntrs, sizeof(Counter));
        rc = VTSS_RC_OK;

        break;
    }
    DHCPV6C_CRIT_EXIT();

    return rc;
}

#define TX_ENCODE_ELAPSE_TIME(w, x, y, z)       do {                                            \
    DHCP6_OPT_CODE_SET((z), OPT_ELAPSED_TIME);                                                  \
    DHCP6_OPT_LEN_SET((z), sizeof(u16));                                                        \
    if ((x)->init_xmt == 0) (x)->init_xmt = (w);                                                \
    if ((w) > (x)->init_xmt) {                                                                  \
        if (((w) - (x)->init_xmt) < 0xFFFF) {                                                   \
            (z)->elapsed_time = htons(VTSS_OS_TICK2MSEC((w) - (x)->init_xmt) / 10);             \
        } else {                                                                                \
            (z)->elapsed_time = 0xFFFF;                                                         \
        }                                                                                       \
    } else if ((w) < (x)->init_xmt) {                                                           \
        if (((x)->init_xmt - (w) + 1) < 0xFFFF) {                                               \
            (z)->elapsed_time = htons(VTSS_OS_TICK2MSEC((x)->init_xmt - (w) + 1) / 10);         \
        } else {                                                                                \
            (z)->elapsed_time = 0xFFFF;                                                         \
        }                                                                                       \
    } else {                                                                                    \
        (z)->elapsed_time = 0x0;                                                                \
    }                                                                                           \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

#define TX_ENCODE_CLIENT_ID(x, y, z)            do {                                            \
    DeviceDuid  *_cid_duid = &(z)->duid;                                                        \
    DHCP6_OPT_CODE_SET((z), OPT_CLIENTID);                                                      \
    _cid_duid->duid_type = htons((x)->duid.duid_type);                                          \
    switch ( (x)->duid.duid_type ) {                                                            \
    case DHCPV6_DUID_TYPE_EN:                                                                   \
        DHCP6_OPT_LEN_SET((z), DHCP6_DUID_EN_LEN(_cid_duid));                                   \
        memcpy(&_cid_duid->type.en, &(x)->duid.type.en, sizeof((x)->duid.type.en));             \
        break;                                                                                  \
    case DHCPV6_DUID_TYPE_LLT:                                                                  \
        DHCP6_OPT_LEN_SET((z), DHCP6_DUID_LLT_LEN(_cid_duid));                                  \
        _cid_duid->type.llt.hardware_type = htons((x)->duid.type.llt.hardware_type);            \
        _cid_duid->type.llt.time = htonl((x)->duid.type.llt.time);                              \
        memcpy(&_cid_duid->type.llt.lla, &(x)->duid.type.llt.lla, sizeof(mesa_mac_t));          \
        break;                                                                                  \
    case DHCPV6_DUID_TYPE_LL:                                                                   \
    default:                                                                                    \
        DHCP6_OPT_LEN_SET((z), DHCP6_DUID_LL_LEN(_cid_duid));                                   \
        _cid_duid->type.ll.hardware_type = htons((x)->duid.type.ll.hardware_type);              \
        memcpy(&_cid_duid->type.ll.lla, &(x)->duid.type.ll.lla, sizeof(mesa_mac_t));            \
        break;                                                                                  \
    }                                                                                           \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

#define TX_ENCODE_SERVER_ID(x, y, z)            do {                                            \
    DeviceDuid  *_sid_duid = &(z)->duid;                                                        \
    DHCP6_OPT_CODE_SET((z), OPT_SERVERID);                                                      \
    _sid_duid->duid_type = htons((x)->duid.duid_type);                                          \
    switch ( (x)->duid.duid_type ) {                                                            \
    case DHCPV6_DUID_TYPE_EN:                                                                   \
        DHCP6_OPT_LEN_SET((z), DHCP6_DUID_EN_LEN(_sid_duid));                                   \
        memcpy(&_sid_duid->type.en, &(x)->duid.type.en, sizeof((x)->duid.type.en));             \
        break;                                                                                  \
    case DHCPV6_DUID_TYPE_LLT:                                                                  \
        DHCP6_OPT_LEN_SET((z), DHCP6_DUID_LLT_LEN(_sid_duid));                                  \
        _sid_duid->type.llt.hardware_type = htons((x)->duid.type.llt.hardware_type);            \
        _sid_duid->type.llt.time = htonl((x)->duid.type.llt.time);                              \
        memcpy(&_sid_duid->type.llt.lla, &(x)->duid.type.llt.lla, sizeof(mesa_mac_t));          \
        break;                                                                                  \
    case DHCPV6_DUID_TYPE_LL:                                                                   \
    default:                                                                                    \
        DHCP6_OPT_LEN_SET((z), DHCP6_DUID_LL_LEN(_sid_duid));                                   \
        _sid_duid->type.ll.hardware_type = htons((x)->duid.type.ll.hardware_type);              \
        memcpy(&_sid_duid->type.ll.lla, &(x)->duid.type.ll.lla, sizeof(mesa_mac_t));            \
        break;                                                                                  \
    }                                                                                           \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

#define TX_ENCODE_RAPID_COMMIT(x, y, z)         do {                                            \
    if ((x)->rapid_commit) {                                                                    \
        DHCP6_OPT_CODE_SET((z), OPT_RAPID_COMMIT);                                              \
        DHCP6_OPT_LEN_SET((z), 0);                                                              \
        DHCP6_OPT_LEN_INC((y), (z));                                                            \
    }                                                                                           \
} while (0)

#define TX_ENCODE_RECONF_ACCEPT(y, z)           do {                                            \
    DHCP6_OPT_CODE_SET((z), OPT_RECONF_ACCEPT);                                                 \
    DHCP6_OPT_LEN_SET((z), 0);                                                                  \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

#define TX_ENCODE_REQUEST_OPTIONS(y, z)         do {                                            \
    size_t  u16sz = sizeof(u16), u32sz = sizeof(u32), orosz = DHCP6_SOL_OPT_REQ_CNT + 1;        \
    if (DHCP6_SOL_OPT_REQ_CNT != 0 &&                                                           \
        VTSS_CALLOC_CAST((z)->options, orosz / u16sz, u32sz) != NULL) {                         \
        u32 *_s = (z)->options;                                                                 \
        DHCP6_OPT_CODE_SET((z), OPT_ORO);                                                       \
        if (DHCP6_SOL_OPT_REQ_CNT % u16sz) {                                                    \
            DHCP6_OPT_LEN_SET((z), u32sz * (orosz / u16sz) - (u32sz / u16sz));                  \
        } else {                                                                                \
            DHCP6_OPT_LEN_SET((z), u32sz * (orosz / u16sz));                                    \
        }                                                                                       \
        DHCP6_SOL_OPT_REQ_ENCODE(_s);                                                           \
        DHCP6_OPT_LEN_INC((y), (z));                                                            \
    } else {                                                                                    \
        (z)->options = NULL;                                                                    \
    }                                                                                           \
} while (0)

#define TX_ENCODE_SRV_IANA(v, w, x, y, z)       do {                                            \
    OptIaAddr   _niadr;                                                                         \
    size_t      _nasz = sizeof(OptIaNa) - sizeof((z)->common) - sizeof((z)->options);           \
    size_t      _niadr_sz = sizeof(OptIaAddr) - sizeof(_niadr.options);                         \
    DHCP6_OPT_CODE_SET((z), OPT_IA_NA);                                                         \
    (z)->iaid = htonl((x)->iaid);                                                               \
    (z)->t1 = (z)->t2 = htonl(0);                                                               \
    if ((w) > 0 && VTSS_CALLOC_CAST((z)->options, (w), _niadr_sz) != NULL && (v)) {             \
        u32 _ncnt = (w);                                                                        \
        u8  *_naptr = (z)->options;                                                             \
        memset(&_niadr, 0x0, sizeof(OptIaAddr));                                                \
        DHCP6_OPT_CODE_SET(&_niadr, OPT_IAADDR);                                                \
        DHCP6_OPT_LEN_SET(&_niadr, _niadr_sz - sizeof(_niadr.common));                          \
        DHCP6_WALK_FWD_LIST((v)->adrs_pool, iter) {                                             \
            if ((*iter).type != IA_TYPE_NA) continue;                                           \
            if ((x)->addrs.t1 && htonl((*iter).t1) > (z)->t1) (z)->t1 = htonl((*iter).t1);      \
            if ((x)->addrs.t2 && htonl((*iter).t2) > (z)->t2) (z)->t2 = htonl((*iter).t2);      \
            DHCP6_ADRS_CPY(&_niadr.address, &(*iter).address);                                  \
            _niadr.preferred_lifetime = htonl((*iter).preferred_lifetime);                      \
            _niadr.valid_lifetime = htonl((*iter).valid_lifetime);                              \
            memcpy(_naptr, &_niadr, _niadr_sz);                                                 \
            if (--_ncnt > 0) _naptr += _niadr_sz;                                               \
        }                                                                                       \
        _nasz += ((w) * _niadr_sz);                                                             \
    } else {                                                                                    \
        (z)->options = NULL;                                                                    \
    }                                                                                           \
    DHCP6_OPT_LEN_SET((z), _nasz);                                                              \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

#define TX_ENCODE_SRV_IATA(v, w, x, y, z)       do {                                            \
    OptIaAddr   _tiadr;                                                                         \
    size_t      _tasz = sizeof(OptIaTa) - sizeof((z)->common) - sizeof((z)->options);           \
    size_t      _tiadr_sz = sizeof(OptIaAddr) - sizeof(_tiadr.options);                         \
    DHCP6_OPT_CODE_SET((z), OPT_IA_TA);                                                         \
    (z)->iaid = htonl((x)->iaid);                                                               \
    if ((w) > 0 && VTSS_CALLOC_CAST((z)->options, (w), _tiadr_sz) != NULL && (v)) {             \
        u32 _tcnt = (w);                                                                        \
        u8  *_taptr = (z)->options;                                                             \
        memset(&_tiadr, 0x0, sizeof(OptIaAddr));                                                \
        DHCP6_OPT_CODE_SET(&_tiadr, OPT_IAADDR);                                                \
        DHCP6_OPT_LEN_SET(&_tiadr, _tiadr_sz - sizeof(_tiadr.common));                          \
        DHCP6_WALK_FWD_LIST((v)->adrs_pool, iter) {                                             \
            if ((*iter).type != IA_TYPE_TA) continue;                                           \
            DHCP6_ADRS_CPY(&_tiadr.address, &(*iter).address);                                  \
            _tiadr.preferred_lifetime = htonl((*iter).preferred_lifetime);                      \
            _tiadr.valid_lifetime = htonl((*iter).valid_lifetime);                              \
            memcpy(_taptr, &_tiadr, _tiadr_sz);                                                 \
            if (--_tcnt > 0) _taptr += _tiadr_sz;                                               \
        }                                                                                       \
        _tasz += ((w) * _tiadr_sz);                                                             \
    } else {                                                                                    \
        (z)->options = NULL;                                                                    \
    }                                                                                           \
    DHCP6_OPT_LEN_SET((z), _tasz);                                                              \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

#define TX_ENCODE_IFX_IANA(w, x, y, z)          do {                                            \
    OptIaAddr   _niadr;                                                                         \
    size_t      _nasz = sizeof(OptIaNa) - sizeof((z)->common) - sizeof((z)->options);           \
    size_t      _niadr_sz = sizeof(OptIaAddr) - sizeof(_niadr.options);                         \
    DHCP6_OPT_CODE_SET((z), OPT_IA_NA);                                                         \
    (z)->iaid = htonl((x)->iaid);                                                               \
    (z)->t1 = (z)->t2 = htonl(0);                                                               \
    if ((w) > 0 && VTSS_CALLOC_CAST((z)->options, (w), _niadr_sz) != NULL) {                    \
        const AddrInfo  *_naadrs = &(x)->addrs;                                                 \
        u32             _ncnt = (w);                                                            \
        u8              *_naptr = (z)->options;                                                 \
        memset(&_niadr, 0x0, sizeof(OptIaAddr));                                                \
        DHCP6_OPT_CODE_SET(&_niadr, OPT_IAADDR);                                                \
        DHCP6_OPT_LEN_SET(&_niadr, _niadr_sz - sizeof(_niadr.common));                          \
        while (_ncnt > 0) {                                                                     \
            if (_naadrs->type == IA_TYPE_NA) {                                                  \
                if ((x)->addrs.t1) (z)->t1 = htonl(_naadrs->t1);                                \
                if ((x)->addrs.t2) (z)->t2 = htonl(_naadrs->t2);                                \
                DHCP6_ADRS_CPY(&_niadr.address, &_naadrs->address);                             \
                _niadr.preferred_lifetime = htonl(_naadrs->preferred_lifetime);                 \
                _niadr.valid_lifetime = htonl(_naadrs->valid_lifetime);                         \
                memcpy(_naptr, &_niadr, _niadr_sz);                                             \
            }                                                                                   \
            if (--_ncnt > 0) _naptr += _niadr_sz;                                               \
        }                                                                                       \
        _nasz += ((w) * _niadr_sz);                                                             \
    } else {                                                                                    \
        (z)->options = NULL;                                                                    \
    }                                                                                           \
    DHCP6_OPT_LEN_SET((z), _nasz);                                                              \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

#define TX_ENCODE_IFX_IATA(w, x, y, z)          do {                                            \
    OptIaAddr   _tiadr;                                                                         \
    size_t      _tasz = sizeof(OptIaTa) - sizeof((z)->common) - sizeof((z)->options);           \
    size_t      _tiadr_sz = sizeof(OptIaAddr) - sizeof(_tiadr.options);                         \
    DHCP6_OPT_CODE_SET((z), OPT_IA_TA);                                                         \
    (z)->iaid = htonl((x)->iaid);                                                               \
    if ((w) > 0 && VTSS_CALLOC_CAST((z)->options, (w), _tiadr_sz) != NULL) {                    \
        const AddrInfo  *_taadrs = &((x)->addrs);                                               \
        u32             _tcnt = (w);                                                            \
        u8              *_taptr = (z)->options;                                                 \
        memset(&_tiadr, 0x0, sizeof(OptIaAddr));                                                \
        DHCP6_OPT_CODE_SET(&_tiadr, OPT_IAADDR);                                                \
        DHCP6_OPT_LEN_SET(&_tiadr, _tiadr_sz - sizeof(_tiadr.common));                          \
        while (_tcnt > 0) {                                                                     \
            if (_taadrs->type == IA_TYPE_TA) {                                                  \
                DHCP6_ADRS_CPY(&_tiadr.address, &_taadrs->address);                             \
                _tiadr.preferred_lifetime = htonl(_taadrs->preferred_lifetime);                 \
                _tiadr.valid_lifetime = htonl(_taadrs->valid_lifetime);                         \
                memcpy(_taptr, &_tiadr, _tiadr_sz);                                             \
            }                                                                                   \
            if (--_tcnt > 0) _taptr += _tiadr_sz;                                               \
        }                                                                                       \
        _tasz += ((w) * _tiadr_sz);                                                             \
    } else {                                                                                    \
        (z)->options = NULL;                                                                    \
    }                                                                                           \
    DHCP6_OPT_LEN_SET((z), _tasz);                                                              \
    DHCP6_OPT_LEN_INC((y), (z));                                                                \
} while (0)

mesa_rc do_tx_solicit(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptOro          oros = {};
    OptElapsedTime  elps;
    OptIaNa         iana;
    OptRapidCommit  rcmt;
#if DHCPV6_DO_AUTHENTICATION
    OptReconfAccept rcfg;
#endif /* DHCPV6_DO_AUTHENTICATION */
    mesa_rc         rc;

    u32             optl, na_cnt;
    ServerRecord    *srcd;
    u8              *opts = nullptr;

    if (!intf) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link) {
        return VTSS_RC_OK;
    }
    rc = VTSS_RC_ERROR;

    srcd = NULL;
    optl = na_cnt = 0;

    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);
    TX_ENCODE_RAPID_COMMIT(intf, optl, &rcmt);
#if DHCPV6_DO_AUTHENTICATION
    TX_ENCODE_RECONF_ACCEPT(optl, &rcfg);
#endif /* DHCPV6_DO_AUTHENTICATION */

    oros.options = NULL;
    TX_ENCODE_REQUEST_OPTIONS(optl, &oros);

    iana.options = NULL;
    TX_ENCODE_SRV_IANA(srcd, na_cnt, intf, optl, &iana);

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        if (intf->rapid_commit) {
            DHCP6_OPT_DO_COPY(ptr, &rcmt, DHCP6_OPT_LEN_GET(&rcmt), NULL, 0, 0);
        }
#if DHCPV6_DO_AUTHENTICATION
        DHCP6_OPT_DO_COPY(ptr, &rcfg, DHCP6_OPT_LEN_GET(&rcfg), NULL, 0, 0);
#endif /* DHCPV6_DO_AUTHENTICATION */
        DHCP6_OPT_DO_COPY(ptr, &oros, DHCP6_OPT_LEN_GET(&oros),
                          oros.options,
                          sizeof(OptCommon),
                          DHCP6_MSG_OPT_LENGTH(&oros.common));
        DHCP6_OPT_DO_COPY(ptr, &iana, sizeof(OptIaNa) - sizeof(iana.options),
                          iana.options,
                          sizeof(OptIaNa) - sizeof(iana.options),
                          DHCP6_MSG_OPT_LENGTH(&iana.common) - (sizeof(u32) * 3));

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6SOLICIT, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (iana.options != NULL) {
        VTSS_FREE(iana.options);
    }
    if (oros.options != NULL) {
        VTSS_FREE(oros.options);
    }

    return rc;
}

mesa_rc do_tx_advertise(Interface *const intf, vtss_tick_count_t ts)
{
    return VTSS_RC_ERROR;
}

mesa_rc do_tx_request(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptServerId     sidx;
    OptOro          oros = {};
    OptElapsedTime  elps;
    OptIaNa         iana;
    OptIaTa         iata;
    mesa_rc         rc;

    u32             optl, na_cnt, ta_cnt;
    ServerRecord    *srcd;
    u8              *opts = nullptr;

    if (!intf) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link) {
        return VTSS_RC_OK;
    }
    rc = VTSS_RC_ERROR;

    optl = na_cnt = ta_cnt = 0;
    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);

    oros.options = NULL;
    TX_ENCODE_REQUEST_OPTIONS(optl, &oros);

    if ((srcd = intf->server) != NULL) {
        TX_ENCODE_SERVER_ID(srcd, optl, &sidx);
        if (!srcd->adrs_pool.empty()) {
            DHCP6_WALK_FWD_LIST(srcd->adrs_pool, iter) {
                if ((*iter).type == IA_TYPE_NA) {
                    ++na_cnt;
                }
                if ((*iter).type == IA_TYPE_TA) {
                    ++ta_cnt;
                }
            }
        }
    }

    iana.options = NULL;
    if (na_cnt) {
        TX_ENCODE_SRV_IANA(srcd, na_cnt, intf, optl, &iana);
    }
    iata.options = NULL;
    if (ta_cnt) {
        TX_ENCODE_SRV_IATA(srcd, ta_cnt, intf, optl, &iata);
    }

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &oros, DHCP6_OPT_LEN_GET(&oros),
                          oros.options,
                          sizeof(OptCommon),
                          DHCP6_MSG_OPT_LENGTH(&oros.common));
        if (srcd) {
            DHCP6_OPT_DO_COPY(ptr, &sidx, DHCP6_OPT_LEN_GET(&sidx), NULL, 0, 0);
        }
        if (na_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iana, sizeof(OptIaNa) - sizeof(iana.options),
                              iana.options,
                              sizeof(OptIaNa) - sizeof(iana.options),
                              DHCP6_MSG_OPT_LENGTH(&iana.common) - (sizeof(u32) * 3));
        }
        if (ta_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iata, sizeof(OptIaTa) - sizeof(iata.options),
                              iata.options,
                              sizeof(OptIaTa) - sizeof(iata.options),
                              DHCP6_MSG_OPT_LENGTH(&iata.common) - sizeof(u32));
        }

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6REQUEST, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (iata.options != NULL) {
        VTSS_FREE(iata.options);
    }
    if (iana.options != NULL) {
        VTSS_FREE(iana.options);
    }
    if (oros.options != NULL) {
        VTSS_FREE(oros.options);
    }

    return rc;
}

mesa_rc do_tx_confirm(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptElapsedTime  elps;
    OptIaNa         iana;
    OptIaTa         iata;
    OptIaAddr       *adr;
    mesa_rc         rc;

    u32             optl, na_cnt, ta_cnt;
    u8              *opts = nullptr;

    if (!intf) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link || DHCP6_ADRS_ISZERO(&intf->addrs.address)) {
        return VTSS_RC_OK;
    }

    rc = VTSS_RC_ERROR;
    optl = na_cnt = ta_cnt = 0;
    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);

    if (intf->addrs.type == IA_TYPE_NA) {
        ++na_cnt;
    }
    if (intf->addrs.type == IA_TYPE_TA) {
        ++ta_cnt;
    }

    /*
        RFC-3315: 18.1.2
        The client SHOULD set the T1 and T2 fields in any IA_NA options,
        and the preferred-lifetime and valid-lifetime fields in the IA Address options to 0,
        as the server will ignore these fields.
    */
    iana.options = NULL;
    if (na_cnt) {
        TX_ENCODE_IFX_IANA(na_cnt, intf, optl, &iana);
        if ((adr = (OptIaAddr *)iana.options) != NULL) {
            /* we have only one address assignment */
            adr->preferred_lifetime = adr->valid_lifetime = 0;
        }
    }
    iata.options = NULL;
    if (ta_cnt) {
        TX_ENCODE_IFX_IATA(ta_cnt, intf, optl, &iata);
        if ((adr = (OptIaAddr *)iata.options) != NULL) {
            /* we have only one address assignment */
            adr->preferred_lifetime = adr->valid_lifetime = 0;
        }
    }

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        if (na_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iana, sizeof(OptIaNa) - sizeof(iana.options),
                              iana.options,
                              sizeof(OptIaNa) - sizeof(iana.options),
                              DHCP6_MSG_OPT_LENGTH(&iana.common) - (sizeof(u32) * 3));
        }
        if (ta_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iata, sizeof(OptIaTa) - sizeof(iata.options),
                              iata.options,
                              sizeof(OptIaTa) - sizeof(iata.options),
                              DHCP6_MSG_OPT_LENGTH(&iata.common) - sizeof(u32));
        }

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6CONFIRM, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (iata.options != NULL) {
        VTSS_FREE(iata.options);
    }
    if (iana.options != NULL) {
        VTSS_FREE(iana.options);
    }

    return rc;
}

mesa_rc do_tx_renew(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptServerId     sidx;
    OptOro          oros = {};
    OptElapsedTime  elps;
    OptIaNa         iana;
    OptIaTa         iata;
    mesa_rc         rc;

    u32             optl, na_cnt, ta_cnt;
    ServerRecord    *srcd;
    u8              *opts = nullptr;

    if (!intf) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link || DHCP6_ADRS_ISZERO(&intf->addrs.address)) {
        return VTSS_RC_OK;
    }

    rc = VTSS_RC_ERROR;
    optl = na_cnt = ta_cnt = 0;
    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);

    oros.options = NULL;
    TX_ENCODE_REQUEST_OPTIONS(optl, &oros);

    if ((srcd = intf->server) != NULL) {
        TX_ENCODE_SERVER_ID(srcd, optl, &sidx);
    }

    if (intf->addrs.type == IA_TYPE_NA) {
        ++na_cnt;
    }
    if (intf->addrs.type == IA_TYPE_TA) {
        ++ta_cnt;
    }

    iana.options = NULL;
    if (na_cnt) {
        TX_ENCODE_IFX_IANA(na_cnt, intf, optl, &iana);
    }
    iata.options = NULL;
    if (ta_cnt) {
        TX_ENCODE_IFX_IATA(ta_cnt, intf, optl, &iata);
    }

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        if (srcd) {
            DHCP6_OPT_DO_COPY(ptr, &sidx, DHCP6_OPT_LEN_GET(&sidx), NULL, 0, 0);
        }
        if (na_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iana, sizeof(OptIaNa) - sizeof(iana.options),
                              iana.options,
                              sizeof(OptIaNa) - sizeof(iana.options),
                              DHCP6_MSG_OPT_LENGTH(&iana.common) - (sizeof(u32) * 3));
        }
        if (ta_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iata, sizeof(OptIaTa) - sizeof(iata.options),
                              iata.options,
                              sizeof(OptIaTa) - sizeof(iata.options),
                              DHCP6_MSG_OPT_LENGTH(&iata.common) - sizeof(u32));
        }
        DHCP6_OPT_DO_COPY(ptr, &oros, DHCP6_OPT_LEN_GET(&oros),
                          oros.options,
                          sizeof(OptCommon),
                          DHCP6_MSG_OPT_LENGTH(&oros.common));

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6RENEW, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (iata.options != NULL) {
        VTSS_FREE(iata.options);
    }
    if (iana.options != NULL) {
        VTSS_FREE(iana.options);
    }
    if (oros.options != NULL) {
        VTSS_FREE(oros.options);
    }

    return rc;
}

mesa_rc do_tx_rebind(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptOro          oros = {};
    OptElapsedTime  elps;
    OptIaNa         iana;
    OptIaTa         iata;
    mesa_rc         rc;

    u32             optl, na_cnt, ta_cnt;
    u8              *opts = nullptr;

    if (!intf) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link || DHCP6_ADRS_ISZERO(&intf->addrs.address)) {
        return VTSS_RC_OK;
    }

    rc = VTSS_RC_ERROR;
    optl = na_cnt = ta_cnt = 0;
    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);

    oros.options = NULL;
    TX_ENCODE_REQUEST_OPTIONS(optl, &oros);

    if (intf->addrs.type == IA_TYPE_NA) {
        ++na_cnt;
    }
    if (intf->addrs.type == IA_TYPE_TA) {
        ++ta_cnt;
    }

    iana.options = NULL;
    if (na_cnt) {
        TX_ENCODE_IFX_IANA(na_cnt, intf, optl, &iana);
    }
    iata.options = NULL;
    if (ta_cnt) {
        TX_ENCODE_IFX_IATA(ta_cnt, intf, optl, &iata);
    }

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        if (na_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iana, sizeof(OptIaNa) - sizeof(iana.options),
                              iana.options,
                              sizeof(OptIaNa) - sizeof(iana.options),
                              DHCP6_MSG_OPT_LENGTH(&iana.common) - (sizeof(u32) * 3));
        }
        if (ta_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iata, sizeof(OptIaTa) - sizeof(iata.options),
                              iata.options,
                              sizeof(OptIaTa) - sizeof(iata.options),
                              DHCP6_MSG_OPT_LENGTH(&iata.common) - sizeof(u32));
        }
        DHCP6_OPT_DO_COPY(ptr, &oros, DHCP6_OPT_LEN_GET(&oros),
                          oros.options,
                          sizeof(OptCommon),
                          DHCP6_MSG_OPT_LENGTH(&oros.common));

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6REBIND, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (iata.options != NULL) {
        VTSS_FREE(iata.options);
    }
    if (iana.options != NULL) {
        VTSS_FREE(iana.options);
    }
    if (oros.options != NULL) {
        VTSS_FREE(oros.options);
    }

    return rc;
}

mesa_rc do_tx_reply(Interface *const intf, vtss_tick_count_t ts)
{
    return VTSS_RC_ERROR;
}

mesa_rc do_tx_release(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptServerId     sidx;
    OptElapsedTime  elps;
    OptIaNa         iana;
    OptIaTa         iata;
    mesa_rc         rc;

    u32             optl, na_cnt, ta_cnt;
    ServerRecord    *srcd;
    u8              *opts = nullptr;

    if (!intf) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link || DHCP6_ADRS_ISZERO(&intf->addrs.address)) {
        return VTSS_RC_OK;
    }

    rc = VTSS_RC_ERROR;
    optl = na_cnt = ta_cnt = 0;
    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);

    if ((srcd = intf->server) != NULL) {
        TX_ENCODE_SERVER_ID(srcd, optl, &sidx);
    }

    if (intf->addrs.type == IA_TYPE_NA) {
        ++na_cnt;
    }
    if (intf->addrs.type == IA_TYPE_TA) {
        ++ta_cnt;
    }

    iana.options = NULL;
    if (na_cnt) {
        TX_ENCODE_IFX_IANA(na_cnt, intf, optl, &iana);
    }
    iata.options = NULL;
    if (ta_cnt) {
        TX_ENCODE_IFX_IATA(ta_cnt, intf, optl, &iata);
    }

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        if (srcd) {
            DHCP6_OPT_DO_COPY(ptr, &sidx, DHCP6_OPT_LEN_GET(&sidx), NULL, 0, 0);
        }
        if (na_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iana, sizeof(OptIaNa) - sizeof(iana.options),
                              iana.options,
                              sizeof(OptIaNa) - sizeof(iana.options),
                              DHCP6_MSG_OPT_LENGTH(&iana.common) - (sizeof(u32) * 3));
        }
        if (ta_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iata, sizeof(OptIaTa) - sizeof(iata.options),
                              iata.options,
                              sizeof(OptIaTa) - sizeof(iata.options),
                              DHCP6_MSG_OPT_LENGTH(&iata.common) - sizeof(u32));
        }

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6RELEASE, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (iata.options != NULL) {
        VTSS_FREE(iata.options);
    }
    if (iana.options != NULL) {
        VTSS_FREE(iana.options);
    }

    return rc;
}

mesa_rc do_tx_decline(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptServerId     sidx;
    OptElapsedTime  elps;
    OptIaNa         iana;
    OptIaTa         iata;
    mesa_rc         rc;

    u32             optl, na_cnt, ta_cnt;
    ServerRecord    *srcd;
    u8              *opts = nullptr;

    if (!intf || !intf->dad) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link || DHCP6_ADRS_ISZERO(&intf->addrs.address)) {
        return VTSS_RC_OK;
    }

    rc = VTSS_RC_ERROR;
    optl = na_cnt = ta_cnt = 0;
    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);

    if ((srcd = intf->server) != NULL) {
        TX_ENCODE_SERVER_ID(srcd, optl, &sidx);
    }

    if (intf->addrs.type == IA_TYPE_NA) {
        ++na_cnt;
    }
    if (intf->addrs.type == IA_TYPE_TA) {
        ++ta_cnt;
    }

    iana.options = NULL;
    if (na_cnt) {
        TX_ENCODE_IFX_IANA(na_cnt, intf, optl, &iana);
    }
    iata.options = NULL;
    if (ta_cnt) {
        TX_ENCODE_IFX_IATA(ta_cnt, intf, optl, &iata);
    }

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        if (srcd) {
            DHCP6_OPT_DO_COPY(ptr, &sidx, DHCP6_OPT_LEN_GET(&sidx), NULL, 0, 0);
        }
        if (na_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iana, sizeof(OptIaNa) - sizeof(iana.options),
                              iana.options,
                              sizeof(OptIaNa) - sizeof(iana.options),
                              DHCP6_MSG_OPT_LENGTH(&iana.common) - (sizeof(u32) * 3));
        }
        if (ta_cnt) {
            DHCP6_OPT_DO_COPY(ptr, &iata, sizeof(OptIaTa) - sizeof(iata.options),
                              iata.options,
                              sizeof(OptIaTa) - sizeof(iata.options),
                              DHCP6_MSG_OPT_LENGTH(&iata.common) - sizeof(u32));
        }

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6DECLINE, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (iata.options != NULL) {
        VTSS_FREE(iata.options);
    }
    if (iana.options != NULL) {
        VTSS_FREE(iana.options);
    }

    return rc;
}

mesa_rc do_tx_reconfigure(Interface *const intf, vtss_tick_count_t ts)
{
    return VTSS_RC_ERROR;
}

mesa_rc do_tx_information_request(Interface *const intf, vtss_tick_count_t ts)
{
    OptClientId     cidx;
    OptServerId     sidx;
    OptElapsedTime  elps;
    OptOro          oros = {};
    mesa_rc         rc;

    u32             optl;
    ServerRecord    *srcd;
    u8              *opts = nullptr;

    if (!intf) {
        return VTSS_RC_ERROR;
    }
    if (!intf->link) {
        return VTSS_RC_OK;
    }
    rc = VTSS_RC_ERROR;

    optl = 0;
    TX_ENCODE_ELAPSE_TIME(ts, intf, optl, &elps);
    TX_ENCODE_CLIENT_ID(intf, optl, &cidx);

    if ((srcd = intf->server) != NULL) {
        TX_ENCODE_SERVER_ID(srcd, optl, &sidx);
    }

    oros.options = NULL;
    TX_ENCODE_REQUEST_OPTIONS(optl, &oros);

    T_D("(TS:" VPRI64u")OPT-LEN:%u", ts, optl);
    if (optl && VTSS_CALLOC_CAST(opts, 1, optl) != NULL) {
        u8  *ptr = opts;

        DHCP6_OPT_DO_COPY(ptr, &elps, DHCP6_OPT_LEN_GET(&elps), NULL, 0, 0);
        DHCP6_OPT_DO_COPY(ptr, &cidx, DHCP6_OPT_LEN_GET(&cidx), NULL, 0, 0);
        if (srcd) {
            DHCP6_OPT_DO_COPY(ptr, &sidx, DHCP6_OPT_LEN_GET(&sidx), NULL, 0, 0);
        }
        DHCP6_OPT_DO_COPY(ptr, &oros, DHCP6_OPT_LEN_GET(&oros), oros.options, sizeof(OptCommon), DHCP6_MSG_OPT_LENGTH(&oros.common));

        rc = transmit(intf->ifidx, NULL, &dhcp6_linkscope_relay_agents_and_servers, DHCP6INFORMATION_REQUEST, intf->xidx, optl, opts);
    }

    if (opts != NULL) {
        VTSS_FREE(opts);
    }
    if (oros.options != NULL) {
        VTSS_FREE(oros.options);
    }

    return rc;
}

static BOOL INTERFACE_copy(Interface *ifdx, const Interface *const intf)
{
    if (!ifdx || !intf) {
        return FALSE;
    }

    DHCP6_INTF_SERVER_RECORD_CPY(ifdx, intf);

#if defined(__GNUC__) && __GNUC__ > 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
    memcpy(ifdx, intf, sizeof(Interface) - sizeof(Interface::server) - sizeof(Interface::srvrs)); // NOLINT
#if defined(__GNUC__) && __GNUC__ > 7
#pragma GCC diagnostic pop
#endif

    return DETERMINE_active_server_record(ifdx);
}

mesa_rc interface_itr(mesa_vid_t ifx, Interface *const intf)
{
    Interface   *ifdx, *vidx;
    mesa_rc     rc = VTSS_RC_ERROR;

    if (!intf) {
        return rc;
    }

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty()) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    vidx = NULL;
    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx <= ifx) {
            continue;
        }

        if (vidx == NULL) {
            vidx = ifdx;
        } else {
            if (vidx->ifidx > ifdx->ifidx) {
                vidx = ifdx;
            }
        }
    }
    if (vidx != NULL) {
        if (INTERFACE_copy(intf, vidx)) {
            rc = VTSS_RC_OK;
        }
    }
    DHCPV6C_CRIT_EXIT();

    return rc;
}

mesa_rc interface_get(mesa_vid_t ifx, Interface *const intf)
{
    Interface   *ifdx;
    mesa_rc     rc = VTSS_RC_ERROR;

    if (!intf) {
        return rc;
    }

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty()) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        if (INTERFACE_copy(intf, ifdx)) {
            rc = VTSS_RC_OK;
        }

        break;
    }
    DHCPV6C_CRIT_EXIT();

    return rc;
}

mesa_rc interface_set(mesa_vid_t ifx, const Interface *const intf)
{
    Interface   *ifdx;
    mesa_rc     rc = VTSS_RC_ERROR;

    if (!intf) {
        return rc;
    }

    DHCPV6C_CRIT_ENTER();
    if (!client_intfs.empty()) {
        BOOL    fnd = FALSE;
        u8      cntr = 0;

        DHCP6_WALK_FWD_LIST(client_intfs, iter) {
            ++cntr;
            if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
                continue;
            }

            if (INTERFACE_copy(ifdx, intf)) {
                fnd = TRUE;
            }
        }

        if (!fnd) {
            if (cntr < client_intfs_max) {
                if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx)) {
                    memset(&ifdx->cntrs, 0x0, sizeof(Counter));
                    if (INTERFACE_copy(ifdx, intf)) {
                        client_intfs.insert_after(client_intfs.before_begin(), (Interface *&)ifdx);
                        rc = VTSS_RC_OK;
                    } else {
                        T_D("INTERFACE_copy Failure");
                        DHCP6_INTF_MEM_FREE(ifdx);
                    }
                } else {
                    T_D("MemAlloc Failure");
                }
            } else {
                T_D("INTF-Table-Full(%u)", cntr);
            }
        } else {
            rc = VTSS_RC_OK;
        }
    } else {
        if (DHCP6_INTF_MEM_CALLOC_CAST(ifdx)) {
            memset(&ifdx->cntrs, 0x0, sizeof(Counter));
            if (INTERFACE_copy(ifdx, intf)) {
                client_intfs.insert_after(client_intfs.before_begin(), (Interface *&)ifdx);
                rc = VTSS_RC_OK;
            } else {
                T_D("INTERFACE_copy Failure");
                DHCP6_INTF_MEM_FREE(ifdx);
            }
        } else {
            T_D("MemAlloc Failure");
        }
    }
    DHCPV6C_CRIT_EXIT();

    return rc;
}

mesa_rc interface_del(mesa_vid_t ifx)
{
    Interface   *ifdx, *ifp;
    mesa_rc     rc = VTSS_RC_ERROR;

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty() ||
        DHCP6_INTF_MEM_CALLOC_CAST(ifp) == NULL) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    for (auto iter = client_intfs.begin(), prev = client_intfs.before_begin(); iter != client_intfs.end(); prev = iter, ++iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        if (INTERFACE_copy(ifp, ifdx)) {
            (void)DHCP6_ipstk_address_del(ifdx);
        }

        DHCP6_INTF_SERVER_RECORD_CLR(ifdx, TRUE);
        client_intfs.erase_after(prev);
        DHCP6_INTF_MEM_FREE(ifdx);
        rc = VTSS_RC_OK;

        break;
    }
    DHCPV6C_CRIT_EXIT();

    if (rc == VTSS_RC_OK) {
        DHCP6_RXMT_INIT_MSG(ifp, DHCP6RELEASE);
        DHCP6_TX_XIDX_GENERATE(ifp);

        rc = do_tx_release(ifp, vtss_current_time());
    }
    DHCP6_INTF_MEM_FREE(ifp);

#ifdef VTSS_SW_OPTION_DNS
    vtss_dns_signal();
#endif /* VTSS_SW_OPTION_DNS */

    return rc;
}

mesa_rc interface_rst(mesa_vid_t ifx)
{
    Interface   *ifdx, *ifp;
    mesa_rc     rc = VTSS_RC_ERROR;

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty() ||
        DHCP6_INTF_MEM_CALLOC_CAST(ifp) == NULL) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        if (INTERFACE_copy(ifp, ifdx)) {
            ifp->dad = ifdx->dad = FALSE;
        }
        rc = VTSS_RC_OK;

        break;
    }
    DHCPV6C_CRIT_EXIT();

    if (rc == VTSS_RC_OK) {
        if (ifp->server) {
            DHCP6_RXMT_INIT_MSG(ifp, DHCP6CONFIRM);
            DHCP6_TX_XIDX_GENERATE(ifp);

            if (do_tx_confirm(ifp, vtss_current_time()) != VTSS_RC_OK) {
                T_D("Failed to do_tx_confirm");
            }
            (void) do_tx_migrate_counter(ifp);
        } else {
            DHCP6_RXMT_INIT_MSG(ifp, DHCP6_MSG_INIT);
        }
        rc = interface_set(ifp->ifidx, ifp);
    }
    DHCP6_INTF_MEM_FREE(ifp);

    return rc;
}

mesa_rc interface_clr(mesa_vid_t ifx, BOOL cntr, BOOL addr)
{
    Interface   *ifdx;
    mesa_rc     rc = VTSS_RC_ERROR;

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty()) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        if (cntr) {
            memset(&ifdx->cntrs, 0x0, sizeof(Counter));
        }
        if (addr) {
            memset(&ifdx->addrs, 0x0, sizeof(AddrInfo));
        }
        rc = VTSS_RC_OK;

        break;
    }
    DHCPV6C_CRIT_EXIT();

    return rc;
}

mesa_rc interface_link(mesa_vid_t ifx, i8 link)
{
    Interface   *ifdx, *ifp;
    mesa_rc     rc = VTSS_RC_ERROR;

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty() ||
        DHCP6_INTF_MEM_CALLOC_CAST(ifp) == NULL) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        if (!INTERFACE_copy(ifp, ifdx)) {
            break;
        }

        rc = VTSS_RC_OK;
        if (link > 0) {
            if (ifdx->link) {
                rc = VTSS_RC_ERROR;
            }

            ifp->link = ifdx->link = TRUE;
        } else if (link < 0) {
            if (!ifdx->link) {
                rc = VTSS_RC_ERROR;
            }

            ifp->link = ifdx->link = FALSE;
        } else {
            rc = VTSS_RC_ERROR;
        }
        T_I("INTF%u-Link%s", ifdx->ifidx,
            rc == VTSS_RC_OK ? (ifdx->link ? "Up" : "Down") : "Unchanged");

        break;
    }
    DHCPV6C_CRIT_EXIT();

    if (rc == VTSS_RC_OK && link > 0) {
        if (ifp->active_msg != DHCP6_MSG_INIT && ifp->server) {
            DHCP6_RXMT_INIT_MSG(ifp, DHCP6CONFIRM);
            DHCP6_TX_XIDX_GENERATE(ifp);
            if (do_tx_confirm(ifp, vtss_current_time()) != VTSS_RC_OK) {
                T_D("Failed to do_tx_confirm");
            }
            (void) do_tx_migrate_counter(ifp);
        } else {
            DHCP6_RXMT_INIT_MSG(ifp, ifp->stateless ? DHCP6INFORMATION_REQUEST : DHCP6SOLICIT);
            DHCP6_TX_XIDX_GENERATE(ifp);
        }
        rc = interface_set(ifp->ifidx, ifp);
    }
    DHCP6_INTF_MEM_FREE(ifp);

    return rc;
}

mesa_rc interface_dad(mesa_vid_t ifx)
{
    Interface   *ifdx, *ifp;
    mesa_rc     rc = VTSS_RC_ERROR;

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty() ||
        DHCP6_INTF_MEM_CALLOC_CAST(ifp) == NULL) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        if (!INTERFACE_copy(ifp, ifdx)) {
            break;
        }

        // Delete the duplicate address we are about to decline
        DHCP6_ipstk_address_del(ifdx);

        T_I("INTF%u-DAD", ifdx->ifidx);
        ifp->dad = ifdx->dad = TRUE;
        rc = VTSS_RC_OK;

        break;
    }
    DHCPV6C_CRIT_EXIT();

    if (rc == VTSS_RC_OK && ifp->active_msg != DHCP6_MSG_INIT) {
        DHCP6_RXMT_INIT_MSG(ifp, DHCP6DECLINE);
        DHCP6_TX_XIDX_GENERATE(ifp);

        if ((rc = do_tx_decline(ifp, vtss_current_time())) == VTSS_RC_OK) {
            (void) do_tx_migrate_counter(ifp);
            rc = interface_set(ifp->ifidx, ifp);
        }
    }
    DHCP6_INTF_MEM_FREE(ifp);

    return rc;
}

mesa_rc interface_flag(mesa_vid_t ifx, BOOL managed, BOOL other)
{
    Interface   *ifdx;
    BOOL        mflg_chg, oflg_chg, stateless;
    MessageType msg_type = DHCP6UNASSIGNED;
    mesa_rc     rc = VTSS_RC_ERROR;

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty()) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    mflg_chg = oflg_chg = FALSE;
    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        rc = VTSS_RC_OK;
        if (ifdx->m_flag != managed) {
            mflg_chg = TRUE;
        }
        if (ifdx->o_flag != other) {
            oflg_chg = TRUE;
        }

        T_D("INTF%u-FLAG:M(%s)/O(%s)",
            ifdx->ifidx,
            managed ? "Y" : "N",
            other ? "Y" : "N");
        if (!mflg_chg && !oflg_chg) {
            break;
        }
        T_I("INTF%u(%s)-MO-CHG:%sM(%s->%s)/%sO(%s->%s)",
            ifdx->ifidx,
            ifdx->stateless ? "Stateless" : "Stateful",
            mflg_chg ? "" : "!",
            ifdx->m_flag ? "Y" : "N",
            managed ? "Y" : "N",
            oflg_chg ? "" : "!",
            ifdx->o_flag ? "Y" : "N",
            other ? "Y" : "N");

        ifdx->m_flag = managed;
        ifdx->o_flag = other;
        stateless = (other && !managed);
        if (ifdx->stateless != stateless) {
            ifdx->stateless = stateless;
            if (stateless) {
                // Stateless DHCP
                if (ifdx->active_msg != DHCP6INFORMATION_REQUEST) {
                    msg_type = DHCP6INFORMATION_REQUEST;
                }
            } else {
                // Stateful DHCP
                if (ifdx->active_msg != DHCP6SOLICIT) {
                    msg_type = DHCP6_MSG_INIT;
                }
            }
        }

        if (msg_type != DHCP6UNASSIGNED) {
            DHCP6_RXMT_INIT_MSG(ifdx, msg_type);
            DHCP6_TX_XIDX_GENERATE(ifdx);

            if (ifdx->stateless && DHCP6_ipstk_address_del(ifdx) == VTSS_RC_OK) {
                memset(&ifdx->addrs, 0x0, sizeof(AddrInfo));
            }
        }

        break;
    }
    DHCPV6C_CRIT_EXIT();

#ifdef VTSS_SW_OPTION_DNS
    if (msg_type == DHCP6_MSG_INIT) {
        vtss_dns_signal();
    }
#endif /* VTSS_SW_OPTION_DNS */

    return rc;
}

mesa_rc interface_rx_cntr_inc(mesa_vid_t ifx, MessageType msg_type, BOOL err, BOOL drop)
{
    Interface   *ifdx;
    Counter     *cntr;
    mesa_rc     rc = VTSS_RC_ERROR;

    T_D("RX_CNT(%u): MSG-%d/%s/%s", ifx, msg_type, err ? "!E" : "OK", drop ? "!D" : "OK");

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty()) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        rc = VTSS_RC_OK;
        cntr = &ifdx->cntrs;

        if (err) {
            cntr->rx_error++;
        }
        if (drop) {
            cntr->rx_drop++;
        }

        switch ( msg_type ) {
        case DHCP6ADVERTISE:
            cntr->rx_advertise++;
            break;
        case DHCP6REPLY:
            cntr->rx_reply++;
            break;
        case DHCP6RECONFIGURE:
            cntr->rx_reconfigure++;
            break;
        default:
            cntr->rx_unknown++;
            break;
        }

        break;
    }
    DHCPV6C_CRIT_EXIT();

    return rc;
}

mesa_rc interface_tx_cntr_inc(mesa_vid_t ifx, MessageType msg_type, BOOL err, BOOL drp)
{
    Interface   *ifdx;
    Counter     *cntr;
    mesa_rc     rc = VTSS_RC_ERROR;

    T_D("TX_CNT(%u): MSG-%d/%s/%s", ifx, msg_type, err ? "!E" : "OK", drp ? "!D" : "OK");

    DHCPV6C_CRIT_ENTER();
    if (client_intfs.empty()) {
        DHCPV6C_CRIT_EXIT();
        return rc;
    }

    DHCP6_WALK_FWD_LIST(client_intfs, iter) {
        if ((ifdx = *iter) == NULL || ifdx->ifidx != ifx) {
            continue;
        }

        rc = VTSS_RC_OK;
        cntr = &ifdx->cntrs;

        if (err) {
            cntr->tx_error++;
        }
        if (drp) {
            cntr->tx_drop++;
        }

        switch ( msg_type ) {
        case DHCP6SOLICIT:
            cntr->tx_solicit++;
            break;
        case DHCP6REQUEST:
            cntr->tx_request++;
            break;
        case DHCP6CONFIRM:
            cntr->tx_confirm++;
            break;
        case DHCP6RENEW:
            cntr->tx_renew++;
            break;
        case DHCP6REBIND:
            cntr->tx_rebind++;
            break;
        case DHCP6RELEASE:
            cntr->tx_release++;
            break;
        case DHCP6DECLINE:
            cntr->tx_decline++;
            break;
        case DHCP6INFORMATION_REQUEST:
            cntr->tx_information_request++;
            break;
        case DHCP6UNASSIGNED:
            break;
        default:
            cntr->tx_unknown++;
            break;
        }

        break;
    }
    DHCPV6C_CRIT_EXIT();

    return rc;
}

static BOOL RXMT_determine(Interface *const ifdx, vtss_tick_count_t ts)
{
    Rxmit   *rxmit;
    BOOL    first;
    u32     calc;

    if (!ifdx) {
        return FALSE;
    }

    /*
        RFC-3315: 14

        First Message:
            RT = IRT + RAND*IRT
        Subsequent Message
            RT = 2*RTprev + RAND*RTprev
               = RTprev + RTprev + RAND*RTprev

        if (MRT && RT > MRT)
            RT = MRT + RAND*MRT

        Unless MRC is zero, the message exchange fails
        once the client has transmitted the message MRC times.
        Unless MRD is zero, the message exchange fails
        once MRD seconds have elapsed since the client first transmitted the message.

        If both MRC and MRD are non-zero, the message exchange fails whenever
        either of the conditions specified in the previous two paragraphs are
        met.

        If both MRC and MRD are zero, the client continues to transmit the
        message until it receives a response.
    */

    first = ifdx->last_xmt ? FALSE : TRUE;
    rxmit = &ifdx->rxmit;

    calc = 0;
    T_D("%u-%s(%u[%u@" VPRI64u"]):PrevRT=%u/RT=" VPRI64u"", ifdx->ifidx, first ? "INIT" : "NEXT", ifdx->active_msg,
        ifdx->xmt_cntr, ifdx->init_xmt, rxmit->rtprev, rxmit->rxmit_timeout);
    if (first) {
        DHCP6_TX_XIDX_GENERATE(ifdx);
        calc = DHCP6_RXMT_SEC_TO_TICK(rxmit->init_rxmit_time);
        DHCP6_RXMT_RAND_SUM(calc, rxmit->max_delay);
    } else {
        if (!rxmit->rtprev || ifdx->xmt_cntr < 2) {
            /* for the case that needs to be sent without RXMT_determine */
            calc = DHCP6_RXMT_SEC_TO_TICK(rxmit->init_rxmit_time);
            DHCP6_RXMT_RAND_SUM(calc, rxmit->max_delay);
            rxmit->rtprev = calc;
        }

        if (ifdx->xmt_cntr > 1) {
            calc = DHCP6_RXMT_SEC_TO_TICK(rxmit->max_rxmit_time);
            if (calc && ifdx->rxmit.rxmit_timeout > (ifdx->init_xmt + calc)) {
                DHCP6_RXMT_RAND_SUM(calc, 0);
            } else {
                calc = rxmit->rtprev;
                DHCP6_RXMT_RAND_SUM(calc, 0);
                calc += rxmit->rtprev;
            }
        }
    }
    DHCP6_RXMT_RT_ASSIGN(rxmit, calc);
    T_D("(TICK:" VPRI64u"/PREV:%u->CALC:%u)RT=" VPRI64u"/IRT=%u/MRT:%u/MRD:" VPRI64u"/MRC:%u/DELAY:%u",
        ts, rxmit->rtprev, calc, rxmit->rxmit_timeout, rxmit->init_rxmit_time,
        rxmit->max_rxmit_time, rxmit->max_rxmit_duration,
        rxmit->max_rxmit_count, rxmit->max_delay);
    rxmit->rtprev = calc;

    if (rxmit->max_rxmit_count && rxmit->max_rxmit_duration) {
        if (!(rxmit->max_rxmit_count > ifdx->xmt_cntr) ||
            !(rxmit->max_rxmit_duration > ts)) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL DO_timer(Interface *const ifdx, vtss_tick_count_t ts)
{
    BOOL    upd_if_db, rv;

    if (!ifdx) {
        return FALSE;
    }

    T_N("(%s:LastMsg<%u>)RXMT-Timeout:" VPRI64u" / Tick:" VPRI64u"",
        ifdx->server ? (!DHCP6_ADRS_ISZERO(&ifdx->server->addrs) ? "ADDR" : "ZERO") : "NULL",
        ifdx->active_msg,
        ifdx->rxmit.rxmit_timeout, ts);
    rv = TRUE;
    upd_if_db = FALSE;
    if (ifdx->active_msg == DHCP6_MSG_INIT) {
        if (ifdx->link && (!ifdx->server || DHCP6_ADRS_ISZERO(&ifdx->server->addrs))) {
            if (ifdx->stateless) {
                DHCP6_RXMT_INIT_MSG(ifdx, DHCP6INFORMATION_REQUEST);
            } else {
                DHCP6_RXMT_INIT_MSG(ifdx, DHCP6SOLICIT);
            }
            upd_if_db = TRUE;
        }
    } else if (ifdx->active_msg == DHCP6SOLICIT) {
        if (!ifdx->xmt_cntr) {
            goto done_timer;
        } else if (ifdx->rxmit.rxmit_timeout > ts) {
            goto done_timer;
        } else if (ifdx->server && !DHCP6_ADRS_ISZERO(&ifdx->server->addrs)) {
            DHCP6_RXMT_INIT_MSG(ifdx, DHCP6REQUEST);
            upd_if_db = TRUE;
        }
    } else if (ifdx->active_msg == DHCP6REQUEST) {
        if (!ifdx->xmt_cntr) {
            goto done_timer;
        } else if (ifdx->rxmit.rxmit_timeout > ts) {
            goto done_timer;
        } else if (ifdx->rxmit.max_rxmit_count <= ifdx->xmt_cntr) {
            DHCP6_RXMT_INIT_MSG(ifdx, DHCP6SOLICIT);
            upd_if_db = TRUE;
        } else {
            if (ifdx->xmt_cntr && !(ifdx->xmt_cntr % 2)) {
                ServerRecord    *cur_srv = ifdx->server;

                if (DETERMINE_alternate_server_record(ifdx)) {
                    if (cur_srv != ifdx->server) {
                        ifdx->last_xmt = 0;
                        upd_if_db = TRUE;
                    }
                }
            }
        }
    } else if (ifdx->active_msg == DHCP6CONFIRM) {
        /*
            If the client receives no responses before the message transmission
            process terminates, as described in section 14, the client SHOULD
            continue to use any IP addresses, using the last known lifetimes for
            those addresses, and SHOULD continue to use any other previously
            obtained configuration parameters.
        */
        if (ifdx->rxmit.max_rxmit_duration <= ts) {
            DHCP6_RXMT_INIT_MSG(ifdx, DHCP6_MSG_INIT);
            upd_if_db = TRUE;
        }
    } else if (ifdx->active_msg == DHCP6RENEW) {
        if (ifdx->rxmit.max_rxmit_duration <= ts) {
            if (ifdx->stateless) {
                goto done_timer;
            }

            if (DETERMINE_active_server_record(ifdx)) {
                if (ifdx->server && !DHCP6_ADRS_ISZERO(&ifdx->server->addrs)) {
                    // Our active server does not respond to RENEW requests, so
                    // clear its address info to avoid selecting it again.
                    DHCP6_ADRS_CLEAR(&ifdx->server->addrs);
                    ifdx->server = NULL;

                    // Start a REBIND sequence
                    DHCP6_RXMT_INIT_MSG(ifdx, DHCP6REBIND);
                    DHCP6_TX_XIDX_GENERATE(ifdx);
                    DHCP6_RXMT_RT_ASSIGN(&ifdx->rxmit, 0);
                    ifdx->last_xmt = ifdx->rxmit.rxmit_timeout;
                } else {
                    DHCP6_RXMT_INIT_MSG(ifdx, DHCP6SOLICIT);
                }
            }
            upd_if_db = TRUE;
        } else if (ifdx->rxmit.rxmit_timeout > ts) {
            goto done_timer;
        }
    } else if (ifdx->active_msg == DHCP6REBIND) {
        if (ifdx->rxmit.max_rxmit_duration > ts) {
            goto done_timer;
        }

        DHCP6_RXMT_INIT_MSG(ifdx, DHCP6SOLICIT);
        upd_if_db = TRUE;
    } else if (ifdx->active_msg == DHCP6RELEASE) {
        if (!ifdx->xmt_cntr) {
            goto done_timer;
        } else if (ifdx->rxmit.rxmit_timeout > ts) {
            goto done_timer;
        } else if (ifdx->rxmit.max_rxmit_count <= ifdx->xmt_cntr) {
            DHCP6_RXMT_INIT_MSG(ifdx, DHCP6_MSG_INIT);
            upd_if_db = TRUE;
        }
    } else if (ifdx->active_msg == DHCP6DECLINE) {
        if (!ifdx->xmt_cntr) {
            goto done_timer;
        } else if (ifdx->rxmit.rxmit_timeout > ts) {
            goto done_timer;
        } else if (ifdx->rxmit.max_rxmit_count <= ifdx->xmt_cntr) {
            DHCP6_RXMT_INIT_MSG(ifdx, DHCP6_MSG_INIT);
            upd_if_db = TRUE;
        }
    } else if (ifdx->active_msg == DHCP6INFORMATION_REQUEST) {
    } else {
        rv = FALSE;
    }

done_timer:
    if (upd_if_db) {
        if (do_tx_migrate_counter(ifdx) == VTSS_RC_OK) {
            rv = (interface_set(ifdx->ifidx, ifdx) == VTSS_RC_OK);
        }
    }

    return rv;
}

static BOOL DO_xmt(Interface *const ifdx, vtss_tick_count_t ts)
{
    BOOL    upd_if_db, rv;

    if (!ifdx) {
        return FALSE;
    }
    if (!ifdx->link ||
        (ifdx->stateless && !ifdx->m_flag && !ifdx->o_flag)) {
        return TRUE;
    }

    rv = TRUE;
    upd_if_db = FALSE;
    if (!ifdx->last_xmt) {
        if ((rv = RXMT_determine(ifdx, ts)) == FALSE) {
            goto done_tx;
        }

        upd_if_db = TRUE;
    }

    if (ifdx->rxmit.rxmit_timeout > ts) {
        goto done_tx;
    }

    if (ifdx->active_msg == DHCP6SOLICIT) {
        rv = (do_tx_solicit(ifdx, ts) == VTSS_RC_OK);
    } else if (ifdx->active_msg == DHCP6REQUEST) {
        rv = (do_tx_request(ifdx, ts) == VTSS_RC_OK);
    } else if (ifdx->active_msg == DHCP6CONFIRM) {
        rv = (do_tx_confirm(ifdx, ts) == VTSS_RC_OK);
    } else if (ifdx->active_msg == DHCP6RENEW) {
        rv = (do_tx_renew(ifdx, ts) == VTSS_RC_OK);
    } else if (ifdx->active_msg == DHCP6REBIND) {
        rv = (do_tx_rebind(ifdx, ts) == VTSS_RC_OK);
    } else if (ifdx->active_msg == DHCP6RELEASE) {
        rv = (do_tx_release(ifdx, ts) == VTSS_RC_OK);
    } else if (ifdx->active_msg == DHCP6DECLINE) {
        rv = (do_tx_decline(ifdx, ts) == VTSS_RC_OK);
    } else if (ifdx->active_msg == DHCP6INFORMATION_REQUEST) {
        rv = (do_tx_information_request(ifdx, ts) == VTSS_RC_OK);
    } else {
        rv = FALSE;
    }

    if (rv) {
        if ((rv = RXMT_determine(ifdx, ts)) == FALSE) {
            goto done_tx;
        }

        ++ifdx->xmt_cntr;
        upd_if_db = TRUE;
    }

done_tx:
    if (upd_if_db) {
        ifdx->last_xmt = ts;
        if (do_tx_migrate_counter(ifdx) == VTSS_RC_OK) {
            rv = (interface_set(ifdx->ifidx, ifdx) == VTSS_RC_OK);
        }
    }

    return rv;
}

mesa_rc tick(void)
{
    mesa_vid_t          vidx;
    Interface           *intf;
    vtss_tick_count_t   current_tick = vtss_current_time();
    mesa_rc             tick_rc = VTSS_RC_OK;

    vidx = VTSS_VID_NULL;
    if (DHCP6_INTF_MEM_CALLOC_CAST(intf)) {
        while (interface_itr(vidx, intf) == VTSS_RC_OK) {
            if (!DO_timer(intf, current_tick)) {
                T_D("DO_timer failed");
            }

            if (!DO_xmt(intf, current_tick)) {
                T_D("DO_tx failed");
            }

            vidx = intf->ifidx;
        }

        DHCP6_INTF_MEM_FREE(intf);
    }

    return tick_rc;
}

void initialize(u8 max_intf_cnt)
{
    critd_init(&DHCPV6C_crit, "dhcp6_client_core", VTSS_MODULE_ID_DHCP6C, CRITD_TYPE_MUTEX);

    DHCPV6C_CRIT_ENTER();
    client_intfs.clear();
    client_intfs_max = max_intf_cnt;
    DHCPV6C_CRIT_EXIT();
}
} /* client */

} /* dhcp6 */
} /* vtss */
